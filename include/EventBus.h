#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Logger.h"

class World;

/**
 * @brief Event dispatch stage aligned with the engine update loop.
 *
 * Phase 1: API skeleton only (no engine wiring yet).
 *
 * Intended pump points (per design doc):
 * - PreFlush  : immediately before World::flushCommandBuffer()
 * - PostFlush : at the end of GameEngine::update() (after PostFlush systems)
 */
enum class EventStage
{
    PreFlush,
    PostFlush
};

/**
 * @brief Opaque subscription handle returned from EventBus::subscribe().
 *
 * Token carries enough information to allow non-templated unsubscribe().
 */
struct SubscriptionToken
{
    std::type_index eventType{typeid(void)};
    std::uint64_t   id{0};

    constexpr bool valid() const noexcept
    {
        return id != 0;
    }
};

class EventBus;

/**
 * @brief RAII helper that automatically unsubscribes on destruction.
 *
 * Phase 1: implemented as a lightweight handle; safe to default-construct.
 */
class ScopedSubscription
{
public:
    ScopedSubscription() = default;

    ScopedSubscription(EventBus& bus, SubscriptionToken token) noexcept;

    ScopedSubscription(const ScopedSubscription&)            = delete;
    ScopedSubscription& operator=(const ScopedSubscription&) = delete;

    ScopedSubscription(ScopedSubscription&& other) noexcept;
    ScopedSubscription& operator=(ScopedSubscription&& other) noexcept;

    ~ScopedSubscription();

    void reset() noexcept;

    [[nodiscard]] SubscriptionToken token() const noexcept
    {
        return m_token;
    }

    [[nodiscard]] bool valid() const noexcept
    {
        return m_bus != nullptr && m_token.valid();
    }

private:
    EventBus*         m_bus{nullptr};
    SubscriptionToken m_token{};
};

/**
 * @brief Type-safe global event bus (staged delivery).
 *
 * Phase 1: API skeleton only. Phase 2 will implement storage, enqueueing,
 * dispatch ordering, mutation safety, exception isolation, and logging.
 *
 * Recommended handler signature: void(const Event&, World&).
 */
class EventBus
{
public:
    EventBus()  = default;
    ~EventBus() = default;

    EventBus(const EventBus&)            = delete;
    EventBus& operator=(const EventBus&) = delete;

    EventBus(EventBus&&)            = default;
    EventBus& operator=(EventBus&&) = default;

    template <typename Event>
    using Handler = std::function<void(const Event&, World&)>;

    /**
     * @brief Subscribe to a specific event type.
     *
     * Phase 2 semantics:
     * - Subscription order is deterministic within the event type.
     * - Subscribing during pump() is allowed; new handlers won't see events already being pumped.
     */
    template <typename Event>
    SubscriptionToken subscribe(Handler<Event> handler)
    {
        auto& queue = getOrCreateQueue<Event>();

        const std::uint64_t id = nextId();
        queue.subscribe(id, std::move(handler));

        SubscriptionToken token{std::type_index(typeid(Event)), id};
        LOG_DEBUG("EventBus: subscribe type='{}' id={} handlers={}", token.eventType.name(), token.id, queue.handlerCount());
        return token;
    }

    /**
     * @brief Convenience subscribe overload for arbitrary callables.
     */
    template <typename Event, typename Func>
    SubscriptionToken subscribe(Func&& func)
    {
        return subscribe<Event>(Handler<Event>(std::forward<Func>(func)));
    }

    /**
     * @brief Unsubscribe using the token returned by subscribe().
     *
     * Safe to call multiple times; unknown tokens are ignored.
     */
    void unsubscribe(SubscriptionToken token) noexcept
    {
        if (!token.valid())
        {
            LOG_WARN("EventBus: unsubscribe called with invalid token");
            return;
        }

        auto it = m_queues.find(token.eventType);
        if (it == m_queues.end())
        {
            LOG_WARN("EventBus: unsubscribe unknown type='{}' id={}", token.eventType.name(), token.id);
            return;
        }

        const bool removed = it->second->unsubscribe(token.id);
        if (!removed)
        {
            LOG_WARN("EventBus: unsubscribe unknown id={} type='{}'", token.id, token.eventType.name());
        }
        else
        {
            LOG_DEBUG("EventBus: unsubscribe type='{}' id={}", token.eventType.name(), token.id);
        }
    }

    /**
     * @brief Enqueue an event instance for staged dispatch.
     *
     * Phase 2 semantics:
     * - FIFO delivery within a single event type.
     * - Events emitted during pump() are delivered on the next pump().
     */
    template <typename Event>
    void emit(const Event& event)
    {
        auto& queue = getOrCreateQueue<Event>();
        queue.emitCopy(event);
        LOG_DEBUG("EventBus: emit(copy) type='{}' pending={}", std::type_index(typeid(Event)).name(), queue.pendingCount());
    }

    /**
     * @brief Enqueue an event instance for staged dispatch (move).
     */
    template <typename Event>
    void emit(Event&& event)
    {
        auto& queue = getOrCreateQueue<Event>();
        queue.emitMove(std::move(event));
        LOG_DEBUG("EventBus: emit(move) type='{}' pending={}", std::type_index(typeid(Event)).name(), queue.pendingCount());
    }

    /**
     * @brief Construct an event in-place and enqueue it.
     */
    template <typename Event, typename... Args>
    void emit(Args&&... args)
    {
        auto& queue = getOrCreateQueue<Event>();
        queue.emitInPlace(std::forward<Args>(args)...);
        LOG_DEBUG("EventBus: emit(emplace) type='{}' pending={}", std::type_index(typeid(Event)).name(), queue.pendingCount());
    }

    /**
     * @brief Dispatch queued events for a given stage, then clear the snapshot for that pump.
     *
     * Phase 2 semantics:
     * - Deterministic ordering across event types: first-use registration order.
     * - Re-entrancy is prevented: calling pump() from inside a handler is ignored.
     */
    void pump(EventStage stage, World& world)
    {
        if (m_isPumping)
        {
            LOG_WARN("EventBus: re-entrant pump ignored (stage={})", stageName(stage));
            return;
        }

        m_isPumping = true;

        const std::size_t typeCountAtStart = m_typeOrder.size();

        // Freeze all current queues so any emit() during pump() is deferred to next pump()
        for (std::size_t i = 0; i < typeCountAtStart; ++i)
        {
            const std::type_index t  = m_typeOrder[i];
            auto                  it = m_queues.find(t);
            if (it != m_queues.end())
            {
                it->second->beginPumpSnapshot();
            }
        }

        std::size_t totalDelivered = 0;
        for (std::size_t i = 0; i < typeCountAtStart; ++i)
        {
            const std::type_index t  = m_typeOrder[i];
            auto                  it = m_queues.find(t);
            if (it == m_queues.end())
            {
                continue;
            }

            totalDelivered += it->second->dispatchSnapshot(stage, world);
            it->second->compactIfNeeded();
        }

        LOG_DEBUG("EventBus: pump stage={} types={} delivered={}", stageName(stage), typeCountAtStart, totalDelivered);
        m_isPumping = false;
    }

private:
    struct IQueue
    {
        virtual ~IQueue() = default;

        virtual bool        unsubscribe(std::uint64_t id) noexcept                    = 0;
        virtual void        beginPumpSnapshot()                                       = 0;
        virtual std::size_t dispatchSnapshot(EventStage stage, World& world) noexcept = 0;
        virtual void        compactIfNeeded() noexcept                                = 0;
    };

    template <typename Event>
    class Queue final : public IQueue
    {
    public:
        void subscribe(std::uint64_t id, Handler<Event> handler)
        {
            m_handlers.push_back(HandlerEntry{id, std::move(handler), true});
        }

        std::size_t handlerCount() const noexcept
        {
            return static_cast<std::size_t>(
                std::count_if(m_handlers.begin(), m_handlers.end(), [](const HandlerEntry& h) { return h.alive; }));
        }

        std::size_t pendingCount() const noexcept
        {
            return m_pending.size();
        }

        void emitCopy(const Event& event)
        {
            m_pending.push_back(event);
        }

        void emitMove(Event&& event)
        {
            m_pending.push_back(std::move(event));
        }

        template <typename... Args>
        void emitInPlace(Args&&... args)
        {
            m_pending.emplace_back(std::forward<Args>(args)...);
        }

        bool unsubscribe(std::uint64_t id) noexcept override
        {
            auto it = std::find_if(m_handlers.begin(),
                                   m_handlers.end(),
                                   [id](const HandlerEntry& h) { return h.id == id; });

            if (it == m_handlers.end())
            {
                return false;
            }

            if (it->alive)
            {
                it->alive      = false;
                m_needsCompact = true;
            }

            return true;
        }

        void beginPumpSnapshot() override
        {
            m_snapshot.swap(m_pending);
            m_pending.clear();
        }

        std::size_t dispatchSnapshot(EventStage stage, World& world) noexcept override
        {
            std::size_t delivered = 0;

            // Snapshot handler count so subscribe() during dispatch doesn't affect current delivery.
            const std::size_t handlerCountAtStart = m_handlers.size();

            for (const auto& ev : m_snapshot)
            {
                for (std::size_t i = 0; i < handlerCountAtStart; ++i)
                {
                    auto& h = m_handlers[i];
                    if (!h.alive)
                    {
                        continue;
                    }

                    try
                    {
                        h.fn(ev, world);
                    }
                    catch (const std::exception& ex)
                    {
                        LOG_ERROR("EventBus: handler exception type='{}' stage={} what='{}'",
                                  std::type_index(typeid(Event)).name(),
                                  stageName(stage),
                                  ex.what());
                    }
                    catch (...)
                    {
                        LOG_ERROR("EventBus: handler unknown exception type='{}' stage={}",
                                  std::type_index(typeid(Event)).name(),
                                  stageName(stage));
                    }

                    ++delivered;
                }
            }

            m_snapshot.clear();
            return delivered;
        }

        void compactIfNeeded() noexcept override
        {
            if (!m_needsCompact)
            {
                return;
            }

            std::vector<HandlerEntry> compacted;
            compacted.reserve(m_handlers.size());
            for (auto& h : m_handlers)
            {
                if (h.alive)
                {
                    compacted.push_back(std::move(h));
                }
            }
            m_handlers.swap(compacted);
            m_needsCompact = false;
        }

    private:
        struct HandlerEntry
        {
            std::uint64_t  id{0};
            Handler<Event> fn{};
            bool           alive{false};
        };

        std::vector<HandlerEntry> m_handlers;
        std::vector<Event>        m_pending;
        std::vector<Event>        m_snapshot;
        bool                      m_needsCompact{false};
    };

    template <typename Event>
    Queue<Event>& getOrCreateQueue()
    {
        const std::type_index t{typeid(Event)};
        auto                  it = m_queues.find(t);
        if (it == m_queues.end())
        {
            auto  q   = std::make_unique<Queue<Event>>();
            auto* ptr = q.get();
            m_queues.emplace(t, std::move(q));
            m_typeOrder.push_back(t);
            return *ptr;
        }

        return *static_cast<Queue<Event>*>(it->second.get());
    }

    static const char* stageName(EventStage stage) noexcept
    {
        switch (stage)
        {
            case EventStage::PreFlush:
                return "PreFlush";
            case EventStage::PostFlush:
                return "PostFlush";
            default:
                return "Unknown";
        }
    }

    std::uint64_t nextId() noexcept
    {
        return ++m_nextId;
    }

    std::uint64_t m_nextId{0};

    std::unordered_map<std::type_index, std::unique_ptr<IQueue>> m_queues;
    std::vector<std::type_index>                                 m_typeOrder;
    bool                                                         m_isPumping{false};
};

inline ScopedSubscription::ScopedSubscription(EventBus& bus, SubscriptionToken token) noexcept
    : m_bus(&bus), m_token(token)
{
}

inline ScopedSubscription::ScopedSubscription(ScopedSubscription&& other) noexcept
    : m_bus(other.m_bus), m_token(other.m_token)
{
    other.m_bus   = nullptr;
    other.m_token = {};
}

inline ScopedSubscription& ScopedSubscription::operator=(ScopedSubscription&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    reset();
    m_bus   = other.m_bus;
    m_token = other.m_token;

    other.m_bus   = nullptr;
    other.m_token = {};

    return *this;
}

inline ScopedSubscription::~ScopedSubscription()
{
    reset();
}

inline void ScopedSubscription::reset() noexcept
{
    if (m_bus != nullptr && m_token.valid())
    {
        m_bus->unsubscribe(m_token);
    }
    m_bus   = nullptr;
    m_token = {};
}
