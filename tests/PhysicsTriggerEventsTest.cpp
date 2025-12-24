#include <gtest/gtest.h>

#include <EventBus.h>
#include <S2DPhysics.h>
#include <World.h>

#include <Components.h>
#include <TriggerEvents.h>

#include <vector>

namespace
{
struct TriggerPair
{
    Entity trigger;
    Entity other;
};
}

TEST(PhysicsTriggerEvents, SensorEnterEmitsTriggerEnter)
{
    World world;
    Systems::S2DPhysics physics;
    physics.bindWorld(&world);

    std::vector<TriggerPair> enters;
    auto token = world.events().subscribe<Physics::TriggerEnter>(
        [&](const Physics::TriggerEnter& ev, World&)
        {
            enters.push_back({ev.triggerEntity, ev.otherEntity});
        });

    (void)token;

    // Sensor trigger entity (static)
    Entity trigger = world.createEntity();
    world.add<Components::CTransform>(trigger, Components::CTransform{{0.0f, 0.0f}, {1.0f, 1.0f}, 0.0f});
    {
        Components::CPhysicsBody2D body;
        body.bodyType = Components::BodyType::Static;
        world.add<Components::CPhysicsBody2D>(trigger, body);
    }
    {
        Components::CCollider2D collider;
        collider.sensor = true;
        collider.createBox(0.5f, 0.5f);
        world.add<Components::CCollider2D>(trigger, collider);
    }

    // Visitor entity (dynamic) starts outside the sensor, then moves into it.
    Entity visitor = world.createEntity();
    world.add<Components::CTransform>(visitor, Components::CTransform{{5.0f, 0.0f}, {1.0f, 1.0f}, 0.0f});
    {
        Components::CPhysicsBody2D body;
        body.bodyType = Components::BodyType::Dynamic;
        world.add<Components::CPhysicsBody2D>(visitor, body);
    }
    {
        Components::CCollider2D collider;
        collider.sensor = false;
        collider.createBox(0.5f, 0.5f);
        world.add<Components::CCollider2D>(visitor, collider);
    }

    // Step once to ensure bodies/shapes are created.
    physics.fixedUpdate(physics.getTimeStep(), world);
    world.events().pump(EventStage::PostFlush, world);
    enters.clear();

    // Move into the sensor and step again to trigger begin overlap.
    auto* visitorTransform = world.components().tryGet<Components::CTransform>(visitor);
    ASSERT_NE(visitorTransform, nullptr);
    visitorTransform->position = {0.0f, 0.0f};

    physics.fixedUpdate(physics.getTimeStep(), world);

    // Deliver staged events.
    world.events().pump(EventStage::PostFlush, world);

    ASSERT_FALSE(enters.empty());
    EXPECT_EQ(enters[0].trigger, trigger);
    EXPECT_EQ(enters[0].other, visitor);
}

TEST(PhysicsTriggerEvents, SensorExitEmitsTriggerExit)
{
    World world;
    Systems::S2DPhysics physics;
    physics.bindWorld(&world);

    std::vector<TriggerPair> exits;
    auto token = world.events().subscribe<Physics::TriggerExit>(
        [&](const Physics::TriggerExit& ev, World&)
        {
            exits.push_back({ev.triggerEntity, ev.otherEntity});
        });

    (void)token;

    // Sensor trigger entity (static)
    Entity trigger = world.createEntity();
    world.add<Components::CTransform>(trigger, Components::CTransform{{0.0f, 0.0f}, {1.0f, 1.0f}, 0.0f});
    {
        Components::CPhysicsBody2D body;
        body.bodyType = Components::BodyType::Static;
        world.add<Components::CPhysicsBody2D>(trigger, body);
    }
    {
        Components::CCollider2D collider;
        collider.sensor = true;
        collider.createBox(0.5f, 0.5f);
        world.add<Components::CCollider2D>(trigger, collider);
    }

    // Visitor entity (dynamic) starts inside the sensor, then moves out.
    Entity visitor = world.createEntity();
    world.add<Components::CTransform>(visitor, Components::CTransform{{0.0f, 0.0f}, {1.0f, 1.0f}, 0.0f});
    {
        Components::CPhysicsBody2D body;
        body.bodyType = Components::BodyType::Dynamic;
        world.add<Components::CPhysicsBody2D>(visitor, body);
    }
    {
        Components::CCollider2D collider;
        collider.sensor = false;
        collider.createBox(0.5f, 0.5f);
        world.add<Components::CCollider2D>(visitor, collider);
    }

    // Step once to ensure bodies/shapes are created and the initial overlap is established.
    physics.fixedUpdate(physics.getTimeStep(), world);
    world.events().pump(EventStage::PostFlush, world);
    exits.clear();

    // Move out of the sensor and step again to trigger end overlap.
    auto* visitorTransform = world.components().tryGet<Components::CTransform>(visitor);
    ASSERT_NE(visitorTransform, nullptr);
    visitorTransform->position = {5.0f, 0.0f};

    physics.fixedUpdate(physics.getTimeStep(), world);
    world.events().pump(EventStage::PostFlush, world);

    ASSERT_FALSE(exits.empty());
    EXPECT_EQ(exits[0].trigger, trigger);
    EXPECT_EQ(exits[0].other, visitor);
}
