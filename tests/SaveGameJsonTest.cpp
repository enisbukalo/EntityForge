#include <gtest/gtest.h>

#include <SaveGame.h>

#include <ExecutablePaths.h>
#include <World.h>

#include <Components.h>

#include <filesystem>

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

TEST(SaveGameJson, RoundTripBuiltInComponents)
{
    const std::string slot = "savegame_json_round_trip";
    const auto        path = resolveSaveFilePath(slot + ".json");

    // Best-effort cleanup from a prior failed run
    std::error_code ec;
    std::filesystem::remove(path, ec);

    World world;

    // Create a target entity first (so it gets a stable save-id "0").
    Entity target = world.createEntity();
    world.add<Components::CName>(target, Components::CName { "Target" });
    world.add<Components::CTransform>(target, Components::CTransform { Vec2(3.0f, 4.0f), Vec2(2.0f, 2.0f), 0.25f });
    world.add<Components::CAudioListener>(target, Components::CAudioListener { 0.8f, 0.4f });
    world.add<Components::CAudioSettings>(target, Components::CAudioSettings { 0.7f, 0.3f, 0.9f });

    // Camera entity that follows target.
    Entity cameraE = world.createEntity();
    world.add<Components::CName>(cameraE, Components::CName { "Camera" });
    Components::CCamera cam;
    cam.name          = "MainCam";
    cam.followTarget  = target;
    cam.followEnabled = true;
    cam.followOffset  = Vec2(1.0f, -2.0f);
    cam.zoom          = 1.25f;
    cam.worldHeight   = 11.0f;
    cam.viewport.left = 0.1f;
    cam.viewport.top  = 0.2f;
    cam.viewport.width = 0.7f;
    cam.viewport.height = 0.6f;
    world.add<Components::CCamera>(cameraE, cam);

    // Collider with multiple fixtures
    Entity colliderE = world.createEntity();
    world.add<Components::CName>(colliderE, Components::CName { "Collider" });
    Components::CCollider2D col;
    col.sensor      = true;
    col.density     = 2.0f;
    col.friction    = 0.15f;
    col.restitution = 0.25f;
    col.addPolygon({ Vec2(0.0f, 0.0f), Vec2(1.0f, 0.0f), Vec2(0.5f, 1.0f) }, 0.05f);
    col.addSegment(Vec2(-1.0f, 0.0f), Vec2(1.0f, 0.0f));
    world.add<Components::CCollider2D>(colliderE, col);

    // Input controller bindings
    Entity inputE = world.createEntity();
    world.add<Components::CName>(inputE, Components::CName { "Input" });
    Components::CInputController ic;
    {
        Components::CInputController::Binding b;
        b.binding.keys.push_back(KeyCode::A);
        b.binding.trigger     = ActionTrigger::Held;
        b.binding.allowRepeat = true;
        ic.bindings["MoveLeft"].push_back(b);
    }
    world.add<Components::CInputController>(inputE, ic);

    // Particle emitter configuration
    Entity particleE = world.createEntity();
    world.add<Components::CName>(particleE, Components::CName { "Particles" });
    Components::CParticleEmitter pe;
    pe.setActive(true);
    pe.setEmissionShape(Components::EmissionShape::Circle);
    pe.setShapeRadius(2.5f);
    pe.setDirection(Vec2(0.0f, 1.0f));
    pe.setSpreadAngle(0.75f);
    pe.setMinSpeed(0.2f);
    pe.setMaxSpeed(0.6f);
    pe.setStartAlpha(0.9f);
    pe.setEndAlpha(0.1f);
    pe.setZIndex(5);
    world.add<Components::CParticleEmitter>(particleE, pe);

    ASSERT_TRUE(Systems::SaveGame::saveWorld(world, slot));
    ASSERT_TRUE(std::filesystem::exists(path));

    World loaded;
    ASSERT_TRUE(Systems::SaveGame::loadWorld(loaded, slot, Systems::LoadMode::ReplaceWorld));

    const Entity loadedTarget  = findEntityByName(loaded, "Target");
    const Entity loadedCamera  = findEntityByName(loaded, "Camera");
    const Entity loadedCollider = findEntityByName(loaded, "Collider");
    const Entity loadedInput   = findEntityByName(loaded, "Input");
    const Entity loadedParticles = findEntityByName(loaded, "Particles");

    ASSERT_TRUE(loadedTarget.isValid());
    ASSERT_TRUE(loadedCamera.isValid());
    ASSERT_TRUE(loadedCollider.isValid());
    ASSERT_TRUE(loadedInput.isValid());
    ASSERT_TRUE(loadedParticles.isValid());

    // Camera follow target resolved via saved-id mapping
    const auto* loadedCam = loaded.get<Components::CCamera>(loadedCamera);
    ASSERT_TRUE(loadedCam != nullptr);
    EXPECT_TRUE(loadedCam->followEnabled);
    EXPECT_EQ(loadedCam->followTarget, loadedTarget);
    EXPECT_FLOAT_EQ(loadedCam->followOffset.x, 1.0f);
    EXPECT_FLOAT_EQ(loadedCam->followOffset.y, -2.0f);

    // Collider fixtures round-trip
    const auto* loadedCol = loaded.get<Components::CCollider2D>(loadedCollider);
    ASSERT_TRUE(loadedCol != nullptr);
    EXPECT_TRUE(loadedCol->sensor);
    EXPECT_FLOAT_EQ(loadedCol->density, 2.0f);
    EXPECT_EQ(loadedCol->fixtures.size(), 2u);

    // Input bindings round-trip
    const auto* loadedIc = loaded.get<Components::CInputController>(loadedInput);
    ASSERT_TRUE(loadedIc != nullptr);
    auto it = loadedIc->bindings.find("MoveLeft");
    ASSERT_TRUE(it != loadedIc->bindings.end());
    ASSERT_FALSE(it->second.empty());
    EXPECT_EQ(it->second[0].binding.trigger, ActionTrigger::Held);
    EXPECT_TRUE(it->second[0].binding.allowRepeat);
    ASSERT_FALSE(it->second[0].binding.keys.empty());
    EXPECT_EQ(it->second[0].binding.keys[0], KeyCode::A);

    // Particle emitter configuration round-trip
    const auto* loadedPe = loaded.get<Components::CParticleEmitter>(loadedParticles);
    ASSERT_TRUE(loadedPe != nullptr);
    EXPECT_TRUE(loadedPe->isActive());
    EXPECT_EQ(loadedPe->getEmissionShape(), Components::EmissionShape::Circle);
    EXPECT_FLOAT_EQ(loadedPe->getShapeRadius(), 2.5f);
    EXPECT_EQ(loadedPe->getZIndex(), 5);

    // Persistent audio settings round-trip
    const auto* loadedSettings = loaded.get<Components::CAudioSettings>(loadedTarget);
    ASSERT_TRUE(loadedSettings != nullptr);
    EXPECT_FLOAT_EQ(loadedSettings->masterVolume, 0.7f);
    EXPECT_FLOAT_EQ(loadedSettings->musicVolume, 0.3f);
    EXPECT_FLOAT_EQ(loadedSettings->sfxVolume, 0.9f);

    // Cleanup
    std::filesystem::remove(path, ec);
    std::filesystem::remove(path.parent_path(), ec);
}
