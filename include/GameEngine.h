#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <memory>
#include <string>
#include <vector>

// Include ECS core
#include <Entity.h>
#include <Vec2.h>
#include <World.h>

// Include system and manager headers
#include <S2DPhysics.h>
#include <SAudio.h>
#include <SCamera.h>
#include <SInput.h>
#include <SObjectives.h>
#include <SParticle.h>
#include <SRenderer.h>
#include <SScript.h>

// Convenient namespace declarations for documentation
// Entity is now a plain struct (just an ID), not in a namespace

// Components namespace - All component types
namespace Components
{
}

// Systems namespace - All system types
namespace Systems
{
}

// Internal namespace - Internal utilities and private implementations
namespace Internal
{
}

namespace Objectives
{
class ObjectiveRegistry;
}

/**
 * @brief Main game engine class handling core game loop and systems
 *
 * @description
 * GameEngine is the central class that manages the game loop, physics updates,
 * and rendering. It provides a fixed timestep update system for consistent
 * physics simulation and handles input processing. The engine uses SRenderer
 * for window management and rendering, abstracting away direct SFML dependencies.
 *
 * The engine organizes code into the following namespaces:
 * - Entity - Entity management
 * - Components - All component types
 * - Systems - All system types
 * - Internal - Internal utilities and private implementations
 */
class GameEngine
{
public:
    /**
     * @brief Constructs a game engine instance
     * @param windowConfig Window initialization configuration
     * @param gravity The global gravity vector (Y-up: positive Y = upward, default: {0.0f, -10.0f})
     * @param subStepCount Number of physics sub-steps per update (default: 6, increase for more stability with many bodies)
     * @param timeStep Fixed time step for physics updates
     * @param pixelsPerMeter Rendering scale for particle system (default: 100.0f)
     */
    GameEngine(const Systems::WindowConfig& windowConfig,
               Vec2                         gravity        = Vec2(0.0f, -10.0f),
               uint8_t                      subStepCount   = 6,
               float                        timeStep       = 1.0f / 60.0f,
               float                        pixelsPerMeter = 100.0f);

    /** @brief Destructor */
    ~GameEngine();

    /**
     * @brief Processes input events
     */
    void readInputs();

    /**
     * @brief Updates game logic and physics
     * @param deltaTime Time elapsed since last update in seconds (variable timestep)
     *
     * Physics updates use fixed timestep internally for stability, while other
     * systems can use the variable deltaTime if needed.
     */
    void update(float deltaTime);

    /**
     * @brief Renders the current game state
     */
    void render();

    /**
     * @brief Checks if the game is still running
     * @return true if the game is running, false otherwise
     */
    bool is_running() const;

    // System and Manager Accessors

    /**
     * @brief Gets the Box2D physics system instance
     * @return Reference to the S2DPhysics singleton
     */
    Systems::S2DPhysics& getPhysics();

    /**
     * @brief Gets the input manager instance
     * @return Reference to the SInput singleton
     */
    Systems::SInput& getInputManager();

    /**
     * @brief Gets the audio system instance
     * @return Reference to the SAudio singleton
     */
    Systems::SAudio& getAudioSystem();

    /**
     * @brief Gets the renderer system instance
     * @return Reference to the SRenderer singleton
     */
    Systems::SRenderer& getRenderer();

    /**
     * @brief Gets the particle system instance
     * @return Reference to the SParticle singleton
     */
    Systems::SParticle& getParticleSystem();

    /**
     * @brief Gets the objectives system instance
     */
    Systems::SObjectives& getObjectivesSystem();

    /**
     * @brief Gets the objective definition registry.
     */
    Objectives::ObjectiveRegistry&       getObjectiveRegistry();
    const Objectives::ObjectiveRegistry& getObjectiveRegistry() const;

    /**
     * @brief Creates a new entity in the world
     * @return The created entity ID
     */
    Entity createEntity()
    {
        return m_world.createEntity();
    }

    /**
     * @brief Gets the central world (entity/component orchestration)
     */
    World& world()
    {
        return m_world;
    }
    const World& world() const
    {
        return m_world;
    }

private:
    std::unique_ptr<Objectives::ObjectiveRegistry> m_objectiveRegistry;

    std::unique_ptr<Systems::SRenderer>   m_renderer;    ///< Renderer owned by engine
    std::unique_ptr<Systems::SInput>      m_input;       ///< Input system owned by engine
    std::unique_ptr<Systems::SScript>     m_script;      ///< Script system owned by engine
    std::unique_ptr<Systems::S2DPhysics>  m_physics;     ///< Physics system owned by engine
    std::unique_ptr<Systems::SCamera>     m_camera;      ///< Camera system owned by engine
    std::unique_ptr<Systems::SParticle>   m_particle;    ///< Particle system owned by engine
    std::unique_ptr<Systems::SAudio>      m_audio;       ///< Audio system owned by engine
    std::unique_ptr<Systems::SObjectives> m_objectives;  ///< Objectives system owned by engine

    std::vector<Systems::ISystem*> m_systemOrder;   ///< Ordered system update list
    World                          m_world;         ///< Central world (registry + lifecycle)
    const uint8_t                  m_subStepCount;  ///< Number of physics sub-steps per update
    const float                    m_timeStep;      ///< Fixed time step for physics updates

    bool  m_gameRunning = false;  ///< Flag indicating if the game is running
    float m_accumulator = 0.0f;   ///< Accumulator for fixed timestep updates

    Vec2 m_gravity;  ///< Global gravity vector

    ScopedSubscription m_windowCloseSubscription;

    /**
     * @brief Registers all component types with stable names for serialization
     */
    void registerComponentTypes();

    /**
     * @brief Validates that registered component type names round-trip to their type_index
     */
    void validateComponentTypeNames();
};

#endif  // GAMEENGINE_H