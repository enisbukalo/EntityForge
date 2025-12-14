#include <gtest/gtest.h>

#include <CParticleEmitter.h>
#include <CTransform.h>
#include <SParticle.h>
#include <World.h>

TEST(SParticleTest, UpdateDoesNotEmitWhenNotInitialized)
{
    World world;

    Entity entity = world.createEntity();
    world.components().add<Components::CTransform>(entity);
    world.components().add<Components::CParticleEmitter>(entity);

    auto* emitter = world.components().get<Components::CParticleEmitter>(entity);
    emitter->setEmissionRate(1.0f);
    emitter->setMaxParticles(10);

    Systems::SParticle system;
    system.update(1.0f, world);

    EXPECT_EQ(emitter->getAliveCount(), 0u);
}

TEST(SParticleTest, UpdateAppliesRotatedEmitterOffsetToSpawnPosition)
{
    World world;

    Entity entity = world.createEntity();
    world.components().add<Components::CTransform>(entity);
    world.components().add<Components::CParticleEmitter>(entity);

    auto* transform = world.components().get<Components::CTransform>(entity);
    transform->position = Vec2(2.0f, 3.0f);

    // 90 degrees CCW: (1, 0) offset becomes (0, 1)
    constexpr float halfPi = 3.14159265358979323846f * 0.5f;
    transform->rotation    = halfPi;

    auto* emitter = world.components().get<Components::CParticleEmitter>(entity);
    emitter->setEmissionShape(Components::EmissionShape::Point);
    emitter->setPositionOffset(Vec2(1.0f, 0.0f));
    emitter->setEmissionRate(1.0f);
    emitter->setMaxParticles(10);

    Systems::SParticle system;
    system.initialize(nullptr);

    system.update(1.0f, world);

    ASSERT_EQ(emitter->getAliveCount(), 1u);
    ASSERT_FALSE(emitter->getParticles().empty());

    const auto& p = emitter->getParticles().front();
    ASSERT_TRUE(p.alive);

    EXPECT_NEAR(p.position.x, 2.0f, 1e-5f);
    EXPECT_NEAR(p.position.y, 4.0f, 1e-5f);
}

TEST(SParticleTest, UpdateKillsExpiredParticlesWhenEmissionDisabled)
{
    World world;

    Entity entity = world.createEntity();
    world.components().add<Components::CTransform>(entity);
    world.components().add<Components::CParticleEmitter>(entity);

    auto* emitter = world.components().get<Components::CParticleEmitter>(entity);
    emitter->setEmissionRate(0.0f);

    Components::Particle particle;
    particle.alive    = true;
    particle.age      = 0.0f;
    particle.lifetime = 0.25f;
    particle.position = Vec2(0.0f, 0.0f);
    particle.velocity = Vec2(0.0f, 0.0f);
    particle.acceleration = Vec2(0.0f, 0.0f);
    emitter->getParticles().push_back(particle);

    Systems::SParticle system;
    system.initialize(nullptr);
    system.update(1.0f, world);

    ASSERT_FALSE(emitter->getParticles().empty());
    EXPECT_FALSE(emitter->getParticles().front().alive);
    EXPECT_EQ(emitter->getAliveCount(), 0u);
}

TEST(SParticleTest, UpdateAdvancesParticlePhysicsAndEffects)
{
    World world;

    Entity entity = world.createEntity();
    world.components().add<Components::CTransform>(entity);
    world.components().add<Components::CParticleEmitter>(entity);

    auto* emitter = world.components().get<Components::CParticleEmitter>(entity);
    emitter->setEmissionRate(0.0f);
    emitter->setFadeOut(true);
    emitter->setStartAlpha(1.0f);
    emitter->setEndAlpha(0.0f);
    emitter->setShrink(true);
    emitter->setShrinkEndScale(0.5f);

    Components::Particle particle;
    particle.alive        = true;
    particle.age          = 0.0f;
    particle.lifetime     = 2.0f;
    particle.position     = Vec2(0.0f, 0.0f);
    particle.velocity     = Vec2(1.0f, 2.0f);
    particle.acceleration = Vec2(0.1f, -0.2f);
    particle.initialSize  = 1.0f;
    particle.size         = 1.0f;
    particle.rotation     = 0.0f;
    particle.rotationSpeed = 1.0f;
    particle.alpha        = 1.0f;
    emitter->getParticles().push_back(particle);

    Systems::SParticle system;
    system.initialize(nullptr);

    constexpr float dt = 0.5f;
    system.update(dt, world);

    const auto& p = emitter->getParticles().front();
    ASSERT_TRUE(p.alive);

    // Velocity integrates acceleration first.
    const float expectedVx = 1.0f + 0.1f * dt;
    const float expectedVy = 2.0f + (-0.2f) * dt;
    EXPECT_NEAR(p.velocity.x, expectedVx, 1e-5f);
    EXPECT_NEAR(p.velocity.y, expectedVy, 1e-5f);

    // Position integrates updated velocity.
    EXPECT_NEAR(p.position.x, expectedVx * dt, 1e-5f);
    EXPECT_NEAR(p.position.y, expectedVy * dt, 1e-5f);

    // Age and rotation advance.
    EXPECT_NEAR(p.age, dt, 1e-5f);
    EXPECT_NEAR(p.rotation, 1.0f * dt, 1e-5f);

    // With fade+shrink enabled, alpha decreases and size shrinks.
    EXPECT_LT(p.alpha, 1.0f);
    EXPECT_LT(p.size, 1.0f);
}
