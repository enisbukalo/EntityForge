#include <gtest/gtest.h>

#include <CCamera.h>
#include <CTransform.h>
#include <SCamera.h>
#include <World.h>

TEST(CameraSystemTest, FollowUpdatesCameraPosition)
{
    World world;

    Entity target = world.createEntity();
    world.components().add<Components::CTransform>(target);
    auto* targetTransform = world.components().get<Components::CTransform>(target);
    targetTransform->position = Vec2(5.0f, 3.0f);

    Entity cameraEntity = world.createEntity();
    world.components().add<Components::CCamera>(cameraEntity);
    auto* camera = world.components().get<Components::CCamera>(cameraEntity);
    camera->followEnabled = true;
    camera->followTarget  = target;
    camera->followOffset  = Vec2(1.0f, 2.0f);
    camera->position      = Vec2(0.0f, 0.0f);

    Systems::SCamera system;
    system.update(0.016f, world);

    EXPECT_FLOAT_EQ(camera->position.x, 6.0f);
    EXPECT_FLOAT_EQ(camera->position.y, 5.0f);
}

TEST(CameraSystemTest, ClampClampsCameraPosition)
{
    World world;

    Entity cameraEntity = world.createEntity();
    world.components().add<Components::CCamera>(cameraEntity);
    auto* camera = world.components().get<Components::CCamera>(cameraEntity);

    camera->position     = Vec2(5.0f, 5.0f);
    camera->clampEnabled = true;
    camera->clampRect.min = Vec2(0.0f, 0.0f);
    camera->clampRect.max = Vec2(4.0f, 3.0f);

    Systems::SCamera system;
    system.update(0.016f, world);

    EXPECT_FLOAT_EQ(camera->position.x, 4.0f);
    EXPECT_FLOAT_EQ(camera->position.y, 3.0f);
}

TEST(CameraSystemTest, ActiveCameraFallsBackToFirstEnabledWhenMainMissing)
{
    World world;

    Entity camEntity = world.createEntity();
    world.components().add<Components::CCamera>(camEntity);
    auto* cam = world.components().get<Components::CCamera>(camEntity);
    cam->name = "Secondary";

    Systems::SCamera system;

    const auto* active = system.getActiveCamera(world);
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active->name, "Secondary");
}

TEST(CameraSystemTest, SetActiveCameraChangesName)
{
    Systems::SCamera system;

    EXPECT_TRUE(system.setActiveCamera("Secondary"));
    EXPECT_FALSE(system.setActiveCamera("Secondary"));
    EXPECT_EQ(system.getActiveCameraName(), "Secondary");
}
