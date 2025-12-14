#include <gtest/gtest.h>

#include <World.h>

#include <components/CMaterial.h>
#include <components/CRenderable.h>
#include <components/CTransform.h>
#include <components/CCollider2D.h>
#include <components/CNativeScript.h>
#include <components/CParticleEmitter.h>
#include <components/CPhysicsBody2D.h>
#include <components/CShader.h>
#include <components/CTexture.h>

#include <systems/SAudio.h>
#include <systems/SCamera.h>
#include <systems/SInput.h>
#include <systems/SParticle.h>
#include <systems/SRenderer.h>
#include <systems/S2DPhysics.h>
#include <systems/SystemLocator.h>

namespace
{
struct TestComponent
{
    int value = 0;

    TestComponent() = default;
    explicit TestComponent(int v) : value(v) {}
};
}

#ifndef NDEBUG
TEST(CoverageCritical, SystemLocatorAssertsWhenMissing)
{
    Systems::SystemLocator::provideInput(nullptr);
    Systems::SystemLocator::providePhysics(nullptr);
    Systems::SystemLocator::provideRenderer(nullptr);
    Systems::SystemLocator::provideParticle(nullptr);
    Systems::SystemLocator::provideAudio(nullptr);
    Systems::SystemLocator::provideCamera(nullptr);

    ASSERT_DEATH((void)Systems::SystemLocator::input(), "Input system not set");
    ASSERT_DEATH((void)Systems::SystemLocator::physics(), "Physics system not set");
    ASSERT_DEATH((void)Systems::SystemLocator::renderer(), "Renderer system not set");
    ASSERT_DEATH((void)Systems::SystemLocator::particle(), "Particle system not set");
    ASSERT_DEATH((void)Systems::SystemLocator::audio(), "Audio system not set");
    ASSERT_DEATH((void)Systems::SystemLocator::camera(), "Camera system not set");
}
#endif

TEST(CoverageCritical, SystemLocatorProvideAndGet)
{
    Systems::SInput     input;
    Systems::S2DPhysics physics;
    Systems::SRenderer  renderer;
    Systems::SParticle  particle;
    Systems::SAudio     audio(0);  // Avoid OpenAL device usage by not creating any sf::Sound objects.
    Systems::SCamera    camera;

    Systems::SystemLocator::provideInput(&input);
    Systems::SystemLocator::providePhysics(&physics);
    Systems::SystemLocator::provideRenderer(&renderer);
    Systems::SystemLocator::provideParticle(&particle);
    Systems::SystemLocator::provideAudio(&audio);
    Systems::SystemLocator::provideCamera(&camera);

    EXPECT_EQ(Systems::SystemLocator::tryInput(), &input);
    EXPECT_EQ(Systems::SystemLocator::tryPhysics(), &physics);
    EXPECT_EQ(Systems::SystemLocator::tryRenderer(), &renderer);
    EXPECT_EQ(Systems::SystemLocator::tryParticle(), &particle);
    EXPECT_EQ(Systems::SystemLocator::tryAudio(), &audio);
    EXPECT_EQ(Systems::SystemLocator::tryCamera(), &camera);

    EXPECT_EQ(&Systems::SystemLocator::input(), &input);
    EXPECT_EQ(&Systems::SystemLocator::physics(), &physics);
    EXPECT_EQ(&Systems::SystemLocator::renderer(), &renderer);
    EXPECT_EQ(&Systems::SystemLocator::particle(), &particle);
    EXPECT_EQ(&Systems::SystemLocator::audio(), &audio);
    EXPECT_EQ(&Systems::SystemLocator::camera(), &camera);

    // Clean up global state for other tests.
    Systems::SystemLocator::provideInput(nullptr);
    Systems::SystemLocator::providePhysics(nullptr);
    Systems::SystemLocator::provideRenderer(nullptr);
    Systems::SystemLocator::provideParticle(nullptr);
    Systems::SystemLocator::provideAudio(nullptr);
    Systems::SystemLocator::provideCamera(nullptr);

    EXPECT_EQ(Systems::SystemLocator::tryInput(), nullptr);
    EXPECT_EQ(Systems::SystemLocator::tryPhysics(), nullptr);
    EXPECT_EQ(Systems::SystemLocator::tryRenderer(), nullptr);
    EXPECT_EQ(Systems::SystemLocator::tryParticle(), nullptr);
    EXPECT_EQ(Systems::SystemLocator::tryAudio(), nullptr);
    EXPECT_EQ(Systems::SystemLocator::tryCamera(), nullptr);
}

TEST(CoverageCritical, RenderAndMaterialComponentGetters)
{
    Components::CRenderable renderable(Components::VisualType::Rectangle);
    (void)renderable.getVisualType();
    (void)renderable.getColor();
    (void)renderable.getZIndex();
    (void)renderable.isVisible();
    (void)renderable.getLineStart();
    (void)renderable.getLineEnd();
    (void)renderable.getLineThickness();

    Components::CMaterial material(Color::White, Components::BlendMode::Alpha, 1.0f);
    (void)material.getTint();
    (void)material.getBlendMode();
    (void)material.getOpacity();
}

TEST(CoverageCritical, WorldTryGetAndDeadQueueCommands)
{
    World world;
    auto  e = world.createEntity();

    // Create some common engine components; later destruction will exercise component-store remove paths.
    world.add<Components::CTransform>(e);
    world.add<Components::CRenderable>(e, Components::VisualType::Rectangle);
    world.add<Components::CMaterial>(e, Color::White);
    world.add<Components::CCollider2D>(e);
    world.add<Components::CPhysicsBody2D>(e);
    world.add<Components::CParticleEmitter>(e);
    world.add<Components::CNativeScript>(e);

    // Hit tryGet instantiations (present and absent).
    EXPECT_NE(world.components().tryGet<Components::CRenderable>(e), nullptr);
    EXPECT_NE(world.components().tryGet<Components::CMaterial>(e), nullptr);
    EXPECT_EQ(world.components().tryGet<Components::CShader>(e), nullptr);
    EXPECT_EQ(world.components().tryGet<Components::CTexture>(e), nullptr);

    // Exercise Registry::tryGet for additional component types.
    EXPECT_EQ(world.components().tryGet<Components::CParticleEmitter>(e), world.tryGet<Components::CParticleEmitter>(e));

    // Queue a command, then kill entity before flushing to exercise logDead paths.
    world.queueAdd<TestComponent>(e, 123);
    world.destroyEntity(e);
    world.flushCommandBuffer();
}

TEST(CoverageCritical, SparseGrowthPath)
{
    World  world;
    Entity last = Entity::null();

    // Force entity indices to grow, exercising sparse-resize behavior for component storage.
    for (int i = 0; i < 1024; ++i)
    {
        last = world.createEntity();
    }

    world.add<TestComponent>(last, 7);
    EXPECT_NE(world.tryGet<TestComponent>(last), nullptr);
}
