#include <GameEngine.h>

#include <Color.h>
#include <Components.h>
#include <InputEvents.h>
#include <Logger.h>
#include <SaveGame.h>
#include <ScriptTypeRegistry.h>
#include <SystemLocator.h>

#include <ObjectiveEvents.h>
#include <ObjectiveRegistry.h>

#include <chrono>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <vector>

#include <UIContext.h>

#include "AudioManager.h"
#include "AudioManagerBehaviour.h"
#include "Barrel.h"
#include "BarrelBehaviour.h"
#include "BarrelSpawner.h"
#include "BarrelSpawnerBehaviour.h"
#include "Boat.h"
#include "BoatBehaviour.h"
#include "CameraController.h"
#include "CameraControllerBehaviour.h"
#include "MainCamera.h"

#include "ExampleHud.h"

using namespace Systems;

// Define Screen Size
static constexpr int   SCREEN_WIDTH  = 1600;
static constexpr int   SCREEN_HEIGHT = 1000;
static const Vec2      GRAVITY{0.0f, 0.0f};  // Y-up, meters/sec^2
static constexpr float PIXELS_PER_METER = 100.0f;

// Derived playfield size in meters (screen dimensions divided by scale)
static constexpr float PLAYFIELD_WIDTH_METERS  = SCREEN_WIDTH / PIXELS_PER_METER;
static constexpr float PLAYFIELD_HEIGHT_METERS = SCREEN_HEIGHT / PIXELS_PER_METER;

static constexpr size_t DEFAULT_BARREL_COUNT = 200;

static void registerExampleScriptTypes()
{
    auto& registry = Serialization::ScriptTypeRegistry::instance();

    (void)registry.registerScript<Example::AudioManagerBehaviour>(Example::AudioManagerBehaviour::kScriptName);
    (void)registry.registerScript<Example::BarrelSpawnerBehaviour>(Example::BarrelSpawnerBehaviour::kScriptName);
    (void)registry.registerScript<Example::BoatBehaviour>(Example::BoatBehaviour::kScriptName);
    (void)registry.registerScript<Example::CameraControllerBehaviour>(Example::CameraControllerBehaviour::kScriptName);
    (void)registry.registerScript<Example::BarrelBehaviour>(Example::BarrelBehaviour::kScriptName);
}

int main()
{
    try
    {
        std::set_terminate(
            []
            {
                std::cerr << "FATAL: std::terminate called\n";
                if (Logger::isInitialized())
                {
                    LOG_ERROR_CONSOLE("FATAL: std::terminate called");
                    Logger::flush();
                }
                std::abort();
            });

        Logger::initialize("logs");
        LOG_INFO_CONSOLE("Starting game initialization...");

        WindowConfig windowConfig;
        windowConfig.width      = SCREEN_WIDTH;
        windowConfig.height     = SCREEN_HEIGHT;
        windowConfig.title      = "Boat Example - ECS Framework";
        windowConfig.vsync      = true;
        windowConfig.frameLimit = 144;

        LOG_INFO_CONSOLE("Creating GameEngine...");

        GameEngine engine(windowConfig, GRAVITY);

        UI::UIContext ui;
        engine.setUIContext(&ui);

        registerExampleScriptTypes();

        LOG_INFO_CONSOLE("Configuring input manager...");

        // Input Manager is already initialized by GameEngine - just disable ImGui passthrough
        engine.getInputManager().setPassToImGui(false);

        LOG_INFO_CONSOLE("Setting up physics...");

        // Set up Box2D physics world (gravity disabled)
        engine.getPhysics().setGravity({0.0f, 0.0f});

        LOG_INFO_CONSOLE("Creating entities...");

        World& world = engine.world();
        (void)Example::spawnAudioManager(world);
        const Entity boat = Example::spawnBoat(world);
        (void)Example::spawnMainCamera(world, boat, PLAYFIELD_HEIGHT_METERS);
        (void)Example::spawnCameraController(world, "Main");
        (void)Example::spawnBarrelSpawner(world, -PLAYFIELD_WIDTH_METERS, PLAYFIELD_WIDTH_METERS, -PLAYFIELD_HEIGHT_METERS, PLAYFIELD_HEIGHT_METERS, DEFAULT_BARREL_COUNT);

        // --- Objectives demo (data-driven) ---
        Entity                   objectiveState;
        std::vector<std::string> objectiveIds;
        {
            std::vector<std::string> errors;
            const bool               ok = engine.getObjectiveRegistry().loadFromDirectory("assets/objectives", &errors);
            if (!ok)
            {
                LOG_ERROR_CONSOLE("ObjectiveRegistry: failed to load objectives");
            }
            else
            {
                LOG_INFO("ObjectiveRegistry: loaded {} objectives", engine.getObjectiveRegistry().size());
            }
            for (const auto& e : errors)
            {
                LOG_ERROR_CONSOLE("{}", e);
            }

            objectiveState = world.createEntity();
            world.components().add<Components::CName>(objectiveState, std::string("ObjectiveState"));
            world.components().add<Components::CObjectives>(objectiveState);

            objectiveIds = {
                "example.objective.boat.move_forward",
                "example.objective.boat.rotate",
                "example.objective.boat.reverse",
                "example.objective.boat.hit_30_barrels",
            };

            for (const auto& id : objectiveIds)
            {
                world.events().emit(Objectives::ObjectiveActivate{std::string(id)});
            }
        }

        Example::ExampleHud hud = Example::ExampleHud::create(ui, engine, objectiveIds);

        // Save/load demo + mouse picking demo.
        // Save files resolve to: <exe_dir>/saved_games/<slot>.json
        LOG_INFO_CONSOLE("Save/Load: F5 = Save, F9 = Load (slot 'main_scene')");
        ScopedSubscription saveLoadSub(
            world.events(),
            world.events().subscribe<InputEvent>(
                [&](const InputEvent& inputEvent, World& eventWorld)
                {
                    if (inputEvent.type == InputEventType::KeyPressed && !inputEvent.key.repeat)
                    {
                        if (inputEvent.key.key == KeyCode::F5)
                        {
                            const bool ok = Systems::SaveGame::saveWorld(eventWorld, "main_scene");
                            LOG_INFO_CONSOLE("SaveGame: {}", ok ? "saved" : "save FAILED");
                            return;
                        }
                        if (inputEvent.key.key == KeyCode::F9)
                        {
                            const bool ok = Systems::SaveGame::loadWorld(eventWorld, "main_scene", Systems::LoadMode::ReplaceWorld);
                            LOG_INFO_CONSOLE("SaveGame: {}", ok ? "loaded" : "load FAILED");
                            return;
                        }
                    }

                    if (inputEvent.type != InputEventType::MouseButtonPressed)
                    {
                        return;
                    }
                    if (inputEvent.mouse.button != MouseButton::Left)
                    {
                        return;
                    }

                    const Vec2 worldPos = Systems::SystemLocator::camera().screenToWorld(eventWorld,
                                                                                         "",
                                                                                         inputEvent.mouse.position);
                    LOG_INFO_CONSOLE("Click px=({}, {}) -> world=({}, {})",
                                     inputEvent.mouse.position.x,
                                     inputEvent.mouse.position.y,
                                     worldPos.x,
                                     worldPos.y);
                }));

        LOG_INFO_CONSOLE("Game initialized!");
        LOG_INFO_CONSOLE("Physics: Box2D v3.1.1 (1 unit = 1 meter, Y-up)");

        auto lastTick = std::chrono::steady_clock::now();

        LOG_INFO_CONSOLE("Entering main loop...");
        LOG_DEBUG_CONSOLE("Renderer window: {}", engine.getRenderer().getWindow() ? "valid" : "null");
        LOG_DEBUG_CONSOLE("Window is open: {}", engine.getRenderer().isWindowOpen() ? "yes" : "no");
        LOG_DEBUG_CONSOLE("Engine is running: {}", engine.is_running() ? "yes" : "no");

        int frameCount = 0;
        while (engine.is_running() && engine.getRenderer().isWindowOpen())
        {
            frameCount++;
            const auto  now    = std::chrono::steady_clock::now();
            const float dt     = std::chrono::duration<float>(now - lastTick).count();
            lastTick           = now;
            const float safeDt = (dt < 0.001f) ? 0.016f : dt;  // Use 60 FPS default if dt is too small

            if (frameCount % 60 == 0)
            {
                LOG_DEBUG_CONSOLE("Reached frame {}", frameCount);
            }

            engine.update(safeDt);

            hud.update(world, boat, objectiveState);

            engine.render();
        }

        LOG_INFO_CONSOLE("Main loop exited after {} frames", frameCount);
        LOG_DEBUG_CONSOLE("Engine is running: {}", engine.is_running() ? "yes" : "no");
        LOG_DEBUG_CONSOLE("Renderer window: {}", engine.getRenderer().getWindow() ? "valid" : "null");
        LOG_DEBUG_CONSOLE("Window is open: {}", engine.getRenderer().isWindowOpen() ? "yes" : "no");

        LOG_INFO_CONSOLE("Exiting normally");
        Logger::shutdown();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FATAL std::exception: " << e.what() << "\n";
        if (Logger::isInitialized())
        {
            LOG_ERROR_CONSOLE("FATAL std::exception: {}", e.what());
            Logger::shutdown();
        }
        return 1;
    }
    catch (...)
    {
        std::cerr << "FATAL unknown exception\n";
        if (Logger::isInitialized())
        {
            LOG_ERROR_CONSOLE("FATAL unknown exception");
            Logger::shutdown();
        }
        return 1;
    }
}
