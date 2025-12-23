#include <gtest/gtest.h>

#include <CNativeScript.h>
#include <SScript.h>
#include <World.h>

namespace
{

struct PingEvent
{
    int delta = 0;
};

class PingListenerScript final : public Components::EventAwareScript
{
public:
    explicit PingListenerScript(int* counter) : m_counter(counter) {}

    void onCreate(Entity /*self*/, World& world) override
    {
        subscribe<PingEvent>(world.events(), [this](const PingEvent& ev, World& /*world*/) {
            if (m_counter)
            {
                *m_counter += ev.delta;
            }
        });
    }

    void onUpdate(float /*deltaTime*/, Entity /*self*/, World& /*world*/) override {}

private:
    int* m_counter = nullptr;
};

}  // namespace

TEST(NativeScriptEventBusSubscriptionTest, ScriptCanSubscribeAndAutoUnsubscribesOnDestruction)
{
    World world;
    Systems::SScript scriptSystem;

    int counter = 0;

    Entity entity = world.createEntity();
    auto*  script = world.components().add<Components::CNativeScript>(entity);
    script->bind<PingListenerScript>(&counter);

    // Drive script lifecycle so onCreate runs and the subscription is registered.
    scriptSystem.update(0.016f, world);

    world.events().emit(PingEvent{3});
    world.events().pump(EventStage::PreFlush, world);
    EXPECT_EQ(counter, 3);

    // Destroying the entity destroys the script instance, which must unsubscribe.
    world.destroyEntity(entity);

    world.events().emit(PingEvent{5});
    world.events().pump(EventStage::PreFlush, world);
    EXPECT_EQ(counter, 3);
}
