#include <gtest/gtest.h>

#include <cmath>
#include <World.h>

#include <components/CNativeScript.h>
#include <components/CPhysicsBody2D.h>
#include <components/CTransform.h>
#include <systems/S2DPhysics.h>
#include <systems/SScript.h>

namespace
{
class SpawnManyScriptsA final : public Components::INativeScript
{
public:
    SpawnManyScriptsA(int* createCountA, int* updateCountA, int* createCountB, int* updateCountB, int spawnCount)
        : m_createCountA(createCountA)
        , m_updateCountA(updateCountA)
        , m_createCountB(createCountB)
        , m_updateCountB(updateCountB)
        , m_spawnCount(spawnCount)
    {
    }

    void onCreate(Entity /*self*/, World& /*world*/) override
    {
        if (m_createCountA)
        {
            ++(*m_createCountA);
        }
    }

    void onUpdate(float /*deltaTime*/, Entity /*self*/, World& world) override
    {
        if (m_updateCountA)
        {
            ++(*m_updateCountA);
        }

        if (m_spawned)
        {
            return;
        }
        m_spawned = true;

        for (int i = 0; i < m_spawnCount; ++i)
        {
            Entity e = world.createEntity();
            auto* s  = world.add<Components::CNativeScript>(e);
            ASSERT_NE(s, nullptr);
            s->bind<SpawnManyScriptsB>(m_createCountB, m_updateCountB);
        }
    }

private:
    class SpawnManyScriptsB final : public Components::INativeScript
    {
    public:
        SpawnManyScriptsB(int* createCountB, int* updateCountB) : m_createCountB(createCountB), m_updateCountB(updateCountB) {}

        void onCreate(Entity /*self*/, World& /*world*/) override
        {
            if (m_createCountB)
            {
                ++(*m_createCountB);
            }
        }

        void onUpdate(float /*deltaTime*/, Entity /*self*/, World& /*world*/) override
        {
            if (m_updateCountB)
            {
                ++(*m_updateCountB);
            }
        }

    private:
        int* m_createCountB{nullptr};
        int* m_updateCountB{nullptr};
    };

    int* m_createCountA{nullptr};
    int* m_updateCountA{nullptr};
    int* m_createCountB{nullptr};
    int* m_updateCountB{nullptr};

    int  m_spawnCount{0};
    bool m_spawned{false};
};
}

TEST(SScriptRegressionTest, SpawningEntitiesWithScriptsDuringUpdateIsSafeAndDeferredToNextTick)
{
    World world;

    int createA = 0;
    int updateA = 0;
    int createB = 0;
    int updateB = 0;

    constexpr int kSpawnCount = 50;

    Entity a = world.createEntity();
    auto*  s = world.add<Components::CNativeScript>(a);
    ASSERT_NE(s, nullptr);
    s->bind<SpawnManyScriptsA>(&createA, &updateA, &createB, &updateB, kSpawnCount);

    Systems::SScript scripts;

    scripts.update(1.0f / 60.0f, world);

    EXPECT_EQ(createA, 1);
    EXPECT_EQ(updateA, 1);
    // Newly spawned scripts should not be created/updated in the same tick due to snapshot semantics.
    EXPECT_EQ(createB, 0);
    EXPECT_EQ(updateB, 0);

    scripts.update(1.0f / 60.0f, world);

    EXPECT_EQ(createA, 1);
    EXPECT_EQ(updateA, 2);
    EXPECT_EQ(createB, kSpawnCount);
    EXPECT_EQ(updateB, kSpawnCount);
}

// Regression test: Spawning entities with scripts during onCreate should not cause
// pointer invalidation crash. This was a real bug where the ComponentStore reallocated
// during onCreate, leaving a dangling pointer before onUpdate was called.
namespace
{
class SpawnerOnCreate final : public Components::INativeScript
{
public:
    SpawnerOnCreate(int* createCount, int* updateCount, int spawnCount)
        : m_createCount(createCount), m_updateCount(updateCount), m_spawnCount(spawnCount)
    {
    }

    void onCreate(Entity /*self*/, World& world) override
    {
        if (m_createCount)
        {
            ++(*m_createCount);
        }

        // Spawn many entities with scripts during onCreate - this can cause reallocation
        for (int i = 0; i < m_spawnCount; ++i)
        {
            Entity e = world.createEntity();
            auto*  s = world.add<Components::CNativeScript>(e);
            ASSERT_NE(s, nullptr);
            s->bind<DummyChildScript>();
        }
    }

    void onUpdate(float /*deltaTime*/, Entity /*self*/, World& /*world*/) override
    {
        if (m_updateCount)
        {
            ++(*m_updateCount);
        }
    }

private:
    class DummyChildScript final : public Components::INativeScript
    {
    public:
        void onUpdate(float /*dt*/, Entity /*self*/, World& /*world*/) override {}
    };

    int* m_createCount{nullptr};
    int* m_updateCount{nullptr};
    int  m_spawnCount{0};
};
}  // namespace

TEST(SScriptRegressionTest, SpawningEntitiesWithScriptsDuringOnCreateDoesNotCrash)
{
    World world;

    int createCount = 0;
    int updateCount = 0;

    // Spawn enough entities to trigger reallocation of the component store
    constexpr int kSpawnCount = 100;

    Entity a = world.createEntity();
    auto*  s = world.add<Components::CNativeScript>(a);
    ASSERT_NE(s, nullptr);
    s->bind<SpawnerOnCreate>(&createCount, &updateCount, kSpawnCount);

    Systems::SScript scripts;

    // This would crash before the fix due to pointer invalidation
    scripts.update(1.0f / 60.0f, world);

    // Parent script's onCreate and onUpdate should both run
    EXPECT_EQ(createCount, 1);
    EXPECT_EQ(updateCount, 1);
}

TEST(S2DPhysicsRegressionTest, FixedUpdateCallbackRegisteredBeforeBodyExistsRunsOnceBodyIsCreated)
{
    World world;

    Entity e = world.createEntity();
    world.add<Components::CTransform>(e);
    world.add<Components::CPhysicsBody2D>(e);

    Systems::S2DPhysics physics;

    int callbackCalls = 0;
    physics.setFixedUpdateCallback(e, [&](float /*dt*/) { ++callbackCalls; });

    // First fixed step: callback runs before body creation, so it should be skipped but retained.
    physics.fixedUpdate(physics.getTimeStep(), world);
    EXPECT_EQ(callbackCalls, 0);

    // Second fixed step: body exists (created during previous update), so callback should run.
    physics.fixedUpdate(physics.getTimeStep(), world);
    EXPECT_EQ(callbackCalls, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// CNativeScript coverage tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(CNativeScriptTest, IsBoundReturnsFalseWhenEmpty)
{
    Components::CNativeScript script;
    EXPECT_FALSE(script.isBound());
}

TEST(CNativeScriptTest, IsBoundReturnsTrueAfterBind)
{
    class DummyScript : public Components::INativeScript
    {
    public:
        void onUpdate(float /*dt*/, Entity /*self*/, World& /*world*/) override {}
    };

    Components::CNativeScript script;
    script.bind<DummyScript>();
    EXPECT_TRUE(script.isBound());
}

TEST(CNativeScriptTest, OnCreateDefaultDoesNotCrash)
{
    // INativeScript::onCreate has a default empty implementation
    class MinimalScript : public Components::INativeScript
    {
    public:
        void onUpdate(float /*dt*/, Entity /*self*/, World& /*world*/) override {}
    };

    MinimalScript script;
    World         world;
    Entity        e = world.createEntity();

    // Should not throw or crash
    script.onCreate(e, world);
}

// ─────────────────────────────────────────────────────────────────────────────
// ISystem interface coverage tests
// ─────────────────────────────────────────────────────────────────────────────

namespace
{
class TestSystem : public Systems::ISystem
{
public:
    void update(float /*deltaTime*/, World& /*world*/) override { updateCalled = true; }

    std::string_view name() const override { return "TestSystem"; }

    bool updateCalled = false;
};
}

TEST(ISystemTest, UsesFixedTimestepDefaultReturnsFalse)
{
    TestSystem system;
    EXPECT_FALSE(system.usesFixedTimestep());
}

TEST(ISystemTest, FixedUpdateDefaultCallsUpdate)
{
    TestSystem system;
    World      world;

    EXPECT_FALSE(system.updateCalled);
    system.fixedUpdate(1.0f / 60.0f, world);
    EXPECT_TRUE(system.updateCalled);
}

TEST(ISystemTest, StageDefaultReturnsPreFlush)
{
    TestSystem system;
    EXPECT_EQ(system.stage(), Systems::UpdateStage::PreFlush);
}

// ─────────────────────────────────────────────────────────────────────────────
// S2DPhysics API coverage tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(S2DPhysicsTest, SetAndGetGravity)
{
    Systems::S2DPhysics physics;
    b2Vec2              newGravity = {0.0f, -20.0f};

    physics.setGravity(newGravity);
    b2Vec2 result = physics.getGravity();

    EXPECT_FLOAT_EQ(result.x, newGravity.x);
    EXPECT_FLOAT_EQ(result.y, newGravity.y);
}

TEST(S2DPhysicsTest, SetAndGetTimeStep)
{
    Systems::S2DPhysics physics;
    float               newTimeStep = 1.0f / 120.0f;

    physics.setTimeStep(newTimeStep);
    EXPECT_FLOAT_EQ(physics.getTimeStep(), newTimeStep);
}

TEST(S2DPhysicsTest, SetAndGetSubStepCount)
{
    Systems::S2DPhysics physics;
    int                 newCount = 8;

    physics.setSubStepCount(newCount);
    EXPECT_EQ(physics.getSubStepCount(), newCount);
}

TEST(S2DPhysicsTest, GetWorldIdIsValid)
{
    Systems::S2DPhysics physics;
    b2WorldId           worldId = physics.getWorldId();

    EXPECT_TRUE(b2World_IsValid(worldId));
}

TEST(S2DPhysicsTest, UsesFixedTimestepReturnsTrue)
{
    Systems::S2DPhysics physics;
    EXPECT_TRUE(physics.usesFixedTimestep());
}

TEST(S2DPhysicsTest, BindWorldSetsInternalPointer)
{
    Systems::S2DPhysics physics;
    World               world;

    // Bind world and call update to verify it doesn't crash
    physics.bindWorld(&world);
    physics.update(1.0f / 60.0f, world);
    // If we got here without crash, the binding worked
}

TEST(S2DPhysicsTest, GetBodyReturnsInvalidForNonExistentEntity)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    b2BodyId bodyId = physics.getBody(e);
    EXPECT_FALSE(b2Body_IsValid(bodyId));
}

TEST(S2DPhysicsTest, DestroyBodyOnNonExistentEntityDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    // Should not crash
    physics.destroyBody(e);
}

TEST(S2DPhysicsTest, ClearFixedUpdateCallbackDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    // Should not crash even when entity has no callback
    physics.clearFixedUpdateCallback(e);
}

TEST(S2DPhysicsTest, RunFixedUpdatesWithNoCallbacksDoesNotCrash)
{
    Systems::S2DPhysics physics;

    // Should not crash
    physics.runFixedUpdates(1.0f / 60.0f);
}

TEST(S2DPhysicsTest, ApplyForceOnNonExistentBodyDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);
    b2Vec2              force = {10.0f, 0.0f};
    b2Vec2              point = {0.0f, 0.0f};

    // Should not crash (body doesn't exist)
    physics.applyForce(e, force, point);
}

TEST(S2DPhysicsTest, ApplyForceToCenterOnNonExistentBodyDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);
    b2Vec2              force = {10.0f, 0.0f};

    // Should not crash
    physics.applyForceToCenter(e, force);
}

TEST(S2DPhysicsTest, ApplyLinearImpulseOnNonExistentBodyDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);
    b2Vec2              impulse = {10.0f, 0.0f};
    b2Vec2              point   = {0.0f, 0.0f};

    // Should not crash
    physics.applyLinearImpulse(e, impulse, point);
}

TEST(S2DPhysicsTest, ApplyLinearImpulseToCenterOnNonExistentBodyDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);
    b2Vec2              impulse = {10.0f, 0.0f};

    // Should not crash
    physics.applyLinearImpulseToCenter(e, impulse);
}

TEST(S2DPhysicsTest, ApplyAngularImpulseOnNonExistentBodyDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    // Should not crash
    physics.applyAngularImpulse(e, 1.0f);
}

TEST(S2DPhysicsTest, ApplyTorqueOnNonExistentBodyDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    // Should not crash
    physics.applyTorque(e, 1.0f);
}

TEST(S2DPhysicsTest, SetLinearVelocityOnNonExistentBodyDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);
    b2Vec2              velocity = {5.0f, 0.0f};

    // Should not crash
    physics.setLinearVelocity(e, velocity);
}

TEST(S2DPhysicsTest, SetAngularVelocityOnNonExistentBodyDoesNotCrash)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    // Should not crash
    physics.setAngularVelocity(e, 1.0f);
}

TEST(S2DPhysicsTest, GetLinearVelocityOnNonExistentBodyReturnsZero)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    b2Vec2 velocity = physics.getLinearVelocity(e);
    EXPECT_FLOAT_EQ(velocity.x, 0.0f);
    EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}

TEST(S2DPhysicsTest, GetAngularVelocityOnNonExistentBodyReturnsZero)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    float omega = physics.getAngularVelocity(e);
    EXPECT_FLOAT_EQ(omega, 0.0f);
}

TEST(S2DPhysicsTest, GetPositionOnNonExistentBodyReturnsZero)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    b2Vec2 pos = physics.getPosition(e);
    EXPECT_FLOAT_EQ(pos.x, 0.0f);
    EXPECT_FLOAT_EQ(pos.y, 0.0f);
}

TEST(S2DPhysicsTest, GetRotationOnNonExistentBodyReturnsZero)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    float rot = physics.getRotation(e);
    EXPECT_FLOAT_EQ(rot, 0.0f);
}

TEST(S2DPhysicsTest, GetForwardVectorOnNonExistentBodyReturnsDefault)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    b2Vec2 forward = physics.getForwardVector(e);
    // Should return a default vector (typically zero)
    EXPECT_TRUE(std::isfinite(forward.x));
    EXPECT_TRUE(std::isfinite(forward.y));
}

TEST(S2DPhysicsTest, GetRightVectorOnNonExistentBodyReturnsDefault)
{
    Systems::S2DPhysics physics;
    Entity              e(1, 1);

    b2Vec2 right = physics.getRightVector(e);
    // Should return a default vector (typically zero)
    EXPECT_TRUE(std::isfinite(right.x));
    EXPECT_TRUE(std::isfinite(right.y));
}
