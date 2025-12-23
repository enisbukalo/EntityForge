#include <gtest/gtest.h>

#define private public
#include "systems/SInput.h"
#undef private

#include "EventBus.h"
#include "World.h"

namespace
{

TEST(SInputEventBusBridgeTest, DispatchCallsLegacySubscribersImmediatelyButEventBusIsStaged)
{
    World          world;
    Systems::SInput input;

    bool legacyCalled = false;
    input.subscribe([&](const InputEvent&) { legacyCalled = true; });

    int busCalls = 0;
    world.events().subscribe<InputEvent>([&](const InputEvent&, World&) { ++busCalls; });

    InputEvent ev{};
    ev.type = InputEventType::KeyPressed;

    input.dispatch(world, ev);

    EXPECT_TRUE(legacyCalled);
    EXPECT_EQ(busCalls, 0);

    world.events().pump(EventStage::PreFlush, world);
    EXPECT_EQ(busCalls, 1);

    world.events().pump(EventStage::PreFlush, world);
    EXPECT_EQ(busCalls, 1);
}

}  // namespace
