#include <gtest/gtest.h>

#include <EventBus.h>
#include <World.h>

namespace
{

struct TestEvent
{
    int value{0};
};

struct OtherEvent
{
    int value{0};
};

}  // namespace

TEST(EventBusTest, SubscribeEmitPumpDelivers)
{
    World    world;
    EventBus bus;

    int count = 0;
    bus.subscribe<TestEvent>(
        [&](const TestEvent& e, World&)
        {
            count += e.value;
        });

    bus.emit<TestEvent>(TestEvent{2});
    bus.pump(EventStage::PreFlush, world);

    EXPECT_EQ(count, 2);
}

TEST(EventBusTest, UnsubscribeStopsDelivery)
{
    World    world;
    EventBus bus;

    int count = 0;
    const auto token = bus.subscribe<TestEvent>(
        [&](const TestEvent&, World&)
        {
            ++count;
        });

    bus.unsubscribe(token);

    bus.emit<TestEvent>(TestEvent{1});
    bus.pump(EventStage::PreFlush, world);

    EXPECT_EQ(count, 0);
}

TEST(EventBusTest, DoubleUnsubscribeAndUnknownUnsubscribeDoNotCrash)
{
    World    world;
    EventBus bus;

    const auto token = bus.subscribe<TestEvent>([](const TestEvent&, World&) {});

    EXPECT_NO_THROW(bus.unsubscribe(token));
    EXPECT_NO_THROW(bus.unsubscribe(token));

    const SubscriptionToken bogus{std::type_index(typeid(TestEvent)), 999999};
    EXPECT_NO_THROW(bus.unsubscribe(bogus));

    (void)world;
}

TEST(EventBusTest, FifoOrderingWithinType)
{
    World    world;
    EventBus bus;

    std::vector<int> seen;
    bus.subscribe<TestEvent>(
        [&](const TestEvent& e, World&)
        {
            seen.push_back(e.value);
        });

    bus.emit<TestEvent>(TestEvent{1});
    bus.emit<TestEvent>(TestEvent{2});
    bus.emit<TestEvent>(TestEvent{3});

    bus.pump(EventStage::PreFlush, world);

    ASSERT_EQ(seen.size(), 3u);
    EXPECT_EQ(seen[0], 1);
    EXPECT_EQ(seen[1], 2);
    EXPECT_EQ(seen[2], 3);
}

TEST(EventBusTest, ExceptionIsolation)
{
    World    world;
    EventBus bus;

    int okCount = 0;

    bus.subscribe<TestEvent>([](const TestEvent&, World&) { throw std::runtime_error("boom"); });
    bus.subscribe<TestEvent>(
        [&](const TestEvent&, World&)
        {
            ++okCount;
        });

    bus.emit<TestEvent>(TestEvent{1});

    EXPECT_NO_THROW(bus.pump(EventStage::PreFlush, world));
    EXPECT_EQ(okCount, 1);
}

TEST(EventBusTest, UnsubscribeDuringDispatchIsSafe)
{
    World    world;
    EventBus bus;

    int first = 0;
    int second = 0;

    SubscriptionToken token2;

    bus.subscribe<TestEvent>(
        [&](const TestEvent&, World&)
        {
            ++first;
            bus.unsubscribe(token2);
        });

    token2 = bus.subscribe<TestEvent>(
        [&](const TestEvent&, World&)
        {
            ++second;
        });

    bus.emit<TestEvent>(TestEvent{1});
    bus.emit<TestEvent>(TestEvent{1});

    bus.pump(EventStage::PreFlush, world);

    EXPECT_EQ(first, 2);
    EXPECT_EQ(second, 0);
}

TEST(EventBusTest, SubscribeDuringDispatchDoesNotAffectCurrentPump)
{
    World    world;
    EventBus bus;

    int a = 0;
    int b = 0;

    bool subscribed = false;

    bus.subscribe<TestEvent>(
        [&](const TestEvent&, World&)
        {
            ++a;
            if (!subscribed)
            {
                subscribed = true;
                bus.subscribe<TestEvent>(
                    [&](const TestEvent&, World&)
                    {
                        ++b;
                    });
            }
        });

    bus.emit<TestEvent>(TestEvent{1});
    bus.pump(EventStage::PreFlush, world);

    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 0);

    bus.emit<TestEvent>(TestEvent{1});
    bus.pump(EventStage::PreFlush, world);

    EXPECT_EQ(a, 2);
    EXPECT_EQ(b, 1);
}

TEST(EventBusTest, EmitDuringDispatchDeliveredNextPump)
{
    World    world;
    EventBus bus;

    int delivered = 0;
    bool emitted = false;

    bus.subscribe<TestEvent>(
        [&](const TestEvent&, World&)
        {
            ++delivered;
            if (!emitted)
            {
                emitted = true;
                bus.emit<TestEvent>(TestEvent{1});
            }
        });

    bus.emit<TestEvent>(TestEvent{1});

    bus.pump(EventStage::PreFlush, world);
    EXPECT_EQ(delivered, 1);

    bus.pump(EventStage::PreFlush, world);
    EXPECT_EQ(delivered, 2);
}

TEST(EventBusTest, ReentrancyIsPreventedAndDoesNotLoseEvents)
{
    World    world;
    EventBus bus;

    int delivered = 0;
    bool emitted = false;

    bus.subscribe<TestEvent>(
        [&](const TestEvent&, World& w)
        {
            ++delivered;
            if (!emitted)
            {
                emitted = true;
                bus.emit<TestEvent>(TestEvent{1});
                // Should be ignored (re-entrant)
                bus.pump(EventStage::PreFlush, w);
            }
        });

    bus.emit<TestEvent>(TestEvent{1});

    bus.pump(EventStage::PreFlush, world);
    EXPECT_EQ(delivered, 1);

    bus.pump(EventStage::PreFlush, world);
    EXPECT_EQ(delivered, 2);
}

TEST(EventBusTest, ScopedSubscriptionUnsubscribesOnDestruction)
{
    World    world;
    EventBus bus;

    int delivered = 0;

    {
        const auto token = bus.subscribe<TestEvent>(
            [&](const TestEvent&, World&)
            {
                ++delivered;
            });
        ScopedSubscription sub(bus, token);
    }

    bus.emit<TestEvent>(TestEvent{1});
    bus.pump(EventStage::PreFlush, world);

    EXPECT_EQ(delivered, 0);
}

TEST(EventBusTest, ScopedSubscriptionMoveTransfersOwnership)
{
    World    world;
    EventBus bus;

    int delivered = 0;

    ScopedSubscription sub2;
    {
        const auto token = bus.subscribe<TestEvent>(
            [&](const TestEvent&, World&)
            {
                ++delivered;
            });

        ScopedSubscription sub1(bus, token);
        sub2 = std::move(sub1);
        EXPECT_FALSE(sub1.valid());
        EXPECT_TRUE(sub2.valid());
    }

    // sub2 still holds the subscription here
    bus.emit<TestEvent>(TestEvent{1});
    bus.pump(EventStage::PreFlush, world);
    EXPECT_EQ(delivered, 1);

    sub2.reset();

    bus.emit<TestEvent>(TestEvent{1});
    bus.pump(EventStage::PreFlush, world);
    EXPECT_EQ(delivered, 1);
}

TEST(EventBusTest, DeterministicCrossTypeOrderIsRegistrationOrder)
{
    World    world;
    EventBus bus;

    std::vector<int> order;

    // Register TestEvent first, then OtherEvent.
    bus.subscribe<TestEvent>(
        [&](const TestEvent& e, World&)
        {
            order.push_back(100 + e.value);
        });
    bus.subscribe<OtherEvent>(
        [&](const OtherEvent& e, World&)
        {
            order.push_back(200 + e.value);
        });

    // Emit in reverse order; pump should still deliver by registration order.
    bus.emit<OtherEvent>(OtherEvent{1});
    bus.emit<TestEvent>(TestEvent{1});

    bus.pump(EventStage::PreFlush, world);

    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], 101);
    EXPECT_EQ(order[1], 201);
}

TEST(EventBusTest, StressManyHandlersManyEvents)
{
    World    world;
    EventBus bus;

    constexpr int handlerCount = 100;
    constexpr int eventCount   = 250;

    int delivered = 0;
    for (int i = 0; i < handlerCount; ++i)
    {
        bus.subscribe<TestEvent>(
            [&](const TestEvent&, World&)
            {
                ++delivered;
            });
    }

    for (int i = 0; i < eventCount; ++i)
    {
        bus.emit<TestEvent>(TestEvent{i});
    }

    bus.pump(EventStage::PreFlush, world);
    EXPECT_EQ(delivered, handlerCount * eventCount);
}

namespace
{
struct DeferredAddEvent
{
    Entity target;
};

struct CDeferredMarker
{
    int value = 0;

    CDeferredMarker() = default;
    explicit CDeferredMarker(int v) : value(v) {}
};
}  // namespace

TEST(EventBusTest, PreFlushPumpAllowsDeferredStructuralChangesVisibleAfterFlush)
{
    World    world;
    EventBus& bus = world.events();

    const Entity e = world.createEntity();

    bus.subscribe<DeferredAddEvent>(
        [&](const DeferredAddEvent& ev, World& w)
        {
            w.components().queueAdd<CDeferredMarker>(ev.target, 123);
            EXPECT_FALSE(w.components().has<CDeferredMarker>(ev.target));
        });

    bus.emit<DeferredAddEvent>({e});

    // Simulate engine loop ordering: PreFlush systems already ran and emitted,
    // then we pump PreFlush events immediately before the flush point.
    bus.pump(EventStage::PreFlush, world);

    // Structural change is still deferred until flush.
    EXPECT_FALSE(world.components().has<CDeferredMarker>(e));

    world.flushCommandBuffer();
    EXPECT_TRUE(world.components().has<CDeferredMarker>(e));
}
