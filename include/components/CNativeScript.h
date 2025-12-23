#ifndef CNATIVESCRIPT_H
#define CNATIVESCRIPT_H

#include <Entity.h>
#include <EventBus.h>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class World;

namespace Components
{

namespace Detail
{

template <typename T, typename = void>
struct HasScriptName : std::false_type
{
};

template <typename T>
struct HasScriptName<T, std::void_t<decltype(T::kScriptName)>> : std::true_type
{
};

}  // namespace Detail

class INativeScript
{
public:
    virtual ~INativeScript() = default;

    virtual void onCreate(Entity self, World& world)
    {
        (void)self;
        (void)world;
    }

    virtual void onUpdate(float deltaTime, Entity self, World& world) = 0;
};

/**
 * @brief Optional base class for scripts that want to subscribe to the global EventBus.
 *
 * Store subscriptions via subscribe(...); they will be automatically unsubscribed when
 * the script instance is destroyed.
 */
class EventAwareScript : public INativeScript
{
public:
    ~EventAwareScript() override = default;

protected:
    template <typename Event, typename Func>
    void subscribe(::EventBus& bus, Func&& handler)
    {
        m_subscriptions.emplace_back(bus, bus.subscribe<Event>(std::forward<Func>(handler)));
    }

    void unsubscribeAll() noexcept
    {
        m_subscriptions.clear();
    }

private:
    std::vector<::ScopedSubscription> m_subscriptions;
};

struct CNativeScript
{
    std::unique_ptr<INativeScript> instance;
    std::string                    scriptTypeName;
    bool                           created = false;

    template <typename T, typename... Args>
    void bind(Args&&... args)
    {
        static_assert(std::is_base_of<INativeScript, T>::value,
                      "CNativeScript::bind requires T to derive from INativeScript");
        instance = std::make_unique<T>(std::forward<Args>(args)...);
        scriptTypeName.clear();

        // If the script type declares a stable name, capture it for serialization.
        // This keeps existing scripts working even if they don't define kScriptName.
        if constexpr (Detail::HasScriptName<T>::value)
        {
            scriptTypeName = std::string(T::kScriptName);
        }
        created = false;
    }

    bool isBound() const
    {
        return static_cast<bool>(instance);
    }
};

}  // namespace Components

#endif  // CNATIVESCRIPT_H
