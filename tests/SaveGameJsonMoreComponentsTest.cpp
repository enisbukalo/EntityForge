#include <gtest/gtest.h>

#include <SaveGame.h>

#include <ExecutablePaths.h>
#include <World.h>

#include <Components.h>

#include <filesystem>
#include <string>

namespace
{

std::filesystem::path resolveSaveFilePath(const std::string& slotFilename)
{
    const auto saveDir = Internal::ExecutablePaths::resolveRelativeToExecutableDir("saved_games");
    return saveDir / slotFilename;
}

Entity findEntityByName(const World& world, const std::string& name)
{
    Entity found = Entity::null();
    world.view<Components::CName>([&](Entity e, const Components::CName& n)
    {
        if (n.name == name)
        {
            found = e;
        }
    });
    return found;
}

}  // namespace

TEST(SaveGameJson, RoundTripMoreBuiltInComponents)
{
    const std::string slot = "savegame_json_round_trip_more_components";
    const auto        path = resolveSaveFilePath(slot + ".json");

    // Best-effort cleanup from a prior failed run
    std::error_code ec;
    std::filesystem::remove(path, ec);

    World world;

    Entity e = world.createEntity();
    world.add<Components::CName>(e, Components::CName{"Extra"});

    // Rendering-ish components (no actual GPU resources needed for serialization)
    Components::CRenderable r;
    r.visualType    = Components::VisualType::Line;
    r.color         = Color{12, 34, 56, 78};
    r.zIndex        = 42;
    r.visible       = false;
    r.lineStart     = Vec2(-1.0f, 2.0f);
    r.lineEnd       = Vec2(3.0f, -4.0f);
    r.lineThickness = 7.5f;
    world.add<Components::CRenderable>(e, r);

    world.add<Components::CTexture>(e, Components::CTexture{"assets/textures/does_not_need_to_exist.png"});
    world.add<Components::CShader>(e, Components::CShader{"assets/shaders/v.glsl", "assets/shaders/f.glsl"});

    Components::CMaterial m;
    m.tint      = Color{9, 8, 7, 6};
    m.blendMode = Components::BlendMode::Multiply;
    m.opacity   = 0.42f;
    world.add<Components::CMaterial>(e, m);

    // Physics config component
    Components::CPhysicsBody2D body;
    body.bodyType      = Components::BodyType::Kinematic;
    body.density       = 2.25f;
    body.friction      = 0.11f;
    body.restitution   = 0.33f;
    body.fixedRotation = true;
    body.linearDamping = 0.5f;
    body.angularDamping = 0.75f;
    body.gravityScale  = 0.25f;
    world.add<Components::CPhysicsBody2D>(e, body);

    // Audio source persists configuration only; runtime requests should be reset on load.
    Components::CAudioSource audio;
    audio.clipId        = "clip:menu_click";
    audio.volume        = 0.66f;
    audio.loop          = true;
    audio.playRequested = true;
    audio.stopRequested = true;
    world.add<Components::CAudioSource>(e, audio);

    ASSERT_TRUE(Systems::SaveGame::saveWorld(world, slot));
    ASSERT_TRUE(std::filesystem::exists(path));

    World loaded;
    ASSERT_TRUE(Systems::SaveGame::loadWorld(loaded, slot, Systems::LoadMode::ReplaceWorld));

    const Entity loadedE = findEntityByName(loaded, "Extra");
    ASSERT_TRUE(loadedE.isValid());

    const auto* loadedR = loaded.get<Components::CRenderable>(loadedE);
    ASSERT_NE(loadedR, nullptr);
    EXPECT_EQ(loadedR->visualType, Components::VisualType::Line);
    EXPECT_EQ(loadedR->color.r, 12);
    EXPECT_EQ(loadedR->color.g, 34);
    EXPECT_EQ(loadedR->color.b, 56);
    EXPECT_EQ(loadedR->color.a, 78);
    EXPECT_EQ(loadedR->zIndex, 42);
    EXPECT_FALSE(loadedR->visible);
    EXPECT_FLOAT_EQ(loadedR->lineStart.x, -1.0f);
    EXPECT_FLOAT_EQ(loadedR->lineStart.y, 2.0f);
    EXPECT_FLOAT_EQ(loadedR->lineEnd.x, 3.0f);
    EXPECT_FLOAT_EQ(loadedR->lineEnd.y, -4.0f);
    EXPECT_FLOAT_EQ(loadedR->lineThickness, 7.5f);

    const auto* loadedTex = loaded.get<Components::CTexture>(loadedE);
    ASSERT_NE(loadedTex, nullptr);
    EXPECT_EQ(loadedTex->texturePath, "assets/textures/does_not_need_to_exist.png");

    const auto* loadedShader = loaded.get<Components::CShader>(loadedE);
    ASSERT_NE(loadedShader, nullptr);
    EXPECT_EQ(loadedShader->vertexShaderPath, "assets/shaders/v.glsl");
    EXPECT_EQ(loadedShader->fragmentShaderPath, "assets/shaders/f.glsl");

    const auto* loadedMat = loaded.get<Components::CMaterial>(loadedE);
    ASSERT_NE(loadedMat, nullptr);
    EXPECT_EQ(loadedMat->tint.r, 9);
    EXPECT_EQ(loadedMat->tint.g, 8);
    EXPECT_EQ(loadedMat->tint.b, 7);
    EXPECT_EQ(loadedMat->tint.a, 6);
    EXPECT_EQ(loadedMat->blendMode, Components::BlendMode::Multiply);
    EXPECT_FLOAT_EQ(loadedMat->opacity, 0.42f);

    const auto* loadedBody = loaded.get<Components::CPhysicsBody2D>(loadedE);
    ASSERT_NE(loadedBody, nullptr);
    EXPECT_EQ(loadedBody->bodyType, Components::BodyType::Kinematic);
    EXPECT_FLOAT_EQ(loadedBody->density, 2.25f);
    EXPECT_FLOAT_EQ(loadedBody->friction, 0.11f);
    EXPECT_FLOAT_EQ(loadedBody->restitution, 0.33f);
    EXPECT_TRUE(loadedBody->fixedRotation);
    EXPECT_FLOAT_EQ(loadedBody->linearDamping, 0.5f);
    EXPECT_FLOAT_EQ(loadedBody->angularDamping, 0.75f);
    EXPECT_FLOAT_EQ(loadedBody->gravityScale, 0.25f);

    const auto* loadedAudio = loaded.get<Components::CAudioSource>(loadedE);
    ASSERT_NE(loadedAudio, nullptr);
    EXPECT_EQ(loadedAudio->clipId, "clip:menu_click");
    EXPECT_FLOAT_EQ(loadedAudio->volume, 0.66f);
    EXPECT_TRUE(loadedAudio->loop);
    EXPECT_FALSE(loadedAudio->playRequested);
    EXPECT_FALSE(loadedAudio->stopRequested);

    std::filesystem::remove(path, ec);
    std::filesystem::remove(path.parent_path(), ec);
}
