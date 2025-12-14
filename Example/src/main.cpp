#include <GameEngine.h>

#include <SFML/Graphics.hpp>

#include <Color.h>
#include <Components.h>
#include <Logger.h>

#include <exception>
#include <iostream>
#include <limits>

#include "AudioManager.h"
#include "BarrelSpawner.h"
#include "Boat.h"

using namespace Systems;

// Define Screen Size
static constexpr int   SCREEN_WIDTH  = 1600;
static constexpr int   SCREEN_HEIGHT = 1000;
static const Vec2      GRAVITY{0.0f, 0.0f};  // Y-up, meters/sec^2
static constexpr float PIXELS_PER_METER = 100.0f;

// Derived playfield size in meters (screen dimensions divided by scale)
static constexpr float PLAYFIELD_WIDTH_METERS  = SCREEN_WIDTH / PIXELS_PER_METER;
static constexpr float PLAYFIELD_HEIGHT_METERS = SCREEN_HEIGHT / PIXELS_PER_METER;

static constexpr size_t DEFAULT_BARREL_COUNT = 20;

static Entity createAudioManager(World& world)
{
    Entity audioManager = world.createEntity();
    auto*  script       = world.components().add<Components::CNativeScript>(audioManager);
    script->bind<Example::AudioManager>();
    return audioManager;
}

static Entity createBarrelSpawner(World& world)
{
    Entity spawner = world.createEntity();
    auto*  script  = world.components().add<Components::CNativeScript>(spawner);
    script->bind<Example::BarrelSpawner>(0.0f, PLAYFIELD_WIDTH_METERS, 0.0f, PLAYFIELD_HEIGHT_METERS, DEFAULT_BARREL_COUNT);
    return spawner;
}

int main()
{
    try
    {
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

        LOG_INFO_CONSOLE("Configuring input manager...");

        // Input Manager is already initialized by GameEngine - just disable ImGui passthrough
        engine.getInputManager().setPassToImGui(false);

        LOG_INFO_CONSOLE("Setting up physics...");

        // Set up Box2D physics world (gravity disabled)
        engine.getPhysics().setGravity({0.0f, 0.0f});

        LOG_INFO_CONSOLE("Creating entities...");

        World& world = engine.world();
        (void)createAudioManager(world);
        (void)Example::spawnBoat(world);
        (void)createBarrelSpawner(world);

        LOG_INFO_CONSOLE("Game initialized!");
        LOG_INFO_CONSOLE("Physics: Box2D v3.1.1 (1 unit = 1 meter, Y-up)");

        sf::Clock clock;
        clock.restart();

        auto* window = engine.getRenderer().getWindow();

        LOG_INFO_CONSOLE("Entering main loop...");
        LOG_DEBUG_CONSOLE("Window pointer: {}", window ? "valid" : "null");
        if (window)
        {
            LOG_DEBUG_CONSOLE("Window is open: {}", window->isOpen() ? "yes" : "no");
        }
        LOG_DEBUG_CONSOLE("Engine is running: {}", engine.is_running() ? "yes" : "no");

        int frameCount = 0;
        while (engine.is_running() && window && window->isOpen())
        {
            frameCount++;
            const float dt     = clock.restart().asSeconds();
            const float safeDt = (dt < 0.001f) ? 0.016f : dt;  // Use 60 FPS default if dt is too small

            if (frameCount % 60 == 0)
            {
                LOG_DEBUG_CONSOLE("Reached frame {}", frameCount);
            }

            engine.update(safeDt);
            engine.render();
        }

        LOG_INFO_CONSOLE("Main loop exited after {} frames", frameCount);
        LOG_DEBUG_CONSOLE("Engine is running: {}", engine.is_running() ? "yes" : "no");
        if (window)
        {
            LOG_DEBUG_CONSOLE("Window is open: {}", window->isOpen() ? "yes" : "no");
        }
        else
        {
            LOG_DEBUG_CONSOLE("Window is null");
        }

        if (window)
        {
            window->close();
        }

        std::cout << "\nGame ended. Press Enter to exit...\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();

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

        std::cerr << "Press Enter to exit...\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
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

        std::cerr << "Press Enter to exit...\n";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
        return 1;
    }
}
