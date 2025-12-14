#include "GameEngine.h"
#include <cassert>
#include <stdexcept>
#include <typeindex>
#include "Logger.h"

// Include all component types for registry registration
#include <Components.h>
#include <SystemLocator.h>

GameEngine::GameEngine(const Systems::WindowConfig& windowConfig, Vec2 gravity, uint8_t subStepCount, float timeStep, float pixelsPerMeter)
    : m_renderer(std::make_unique<Systems::SRenderer>()),
      m_input(std::make_unique<Systems::SInput>()),
      m_script(std::make_unique<Systems::SScript>()),
      m_physics(std::make_unique<Systems::S2DPhysics>()),
      m_camera(std::make_unique<Systems::SCamera>()),
      m_particle(std::make_unique<Systems::SParticle>()),
      m_audio(std::make_unique<Systems::SAudio>()),
      m_subStepCount(subStepCount),
      m_timeStep(1.0f / 60.0f),
      m_gravity(gravity)
{
    Systems::SystemLocator::provideRenderer(m_renderer.get());
    Systems::SystemLocator::provideInput(m_input.get());
    Systems::SystemLocator::providePhysics(m_physics.get());
    Systems::SystemLocator::provideCamera(m_camera.get());
    Systems::SystemLocator::provideParticle(m_particle.get());
    Systems::SystemLocator::provideAudio(m_audio.get());

    // Allow physics system to resolve component data without auxiliary maps
    m_physics->bindWorld(&m_world);

    // Register component type names for diagnostics
    registerComponentTypes();

    // Initialize the async logger
    Logger::initialize();

    LOG_INFO("GameEngine initialized");
    LOG_INFO("Window size: {}x{}", windowConfig.width, windowConfig.height);
    LOG_INFO("SubSteps: {}, TimeStep: {}", static_cast<int>(subStepCount), m_timeStep);
    if (timeStep != m_timeStep)
    {
        LOG_WARN("Ignoring requested timeStep {} and enforcing fixed 60Hz ({}).", timeStep, m_timeStep);
    }

    // Initialize renderer
    if (!m_renderer->initialize(windowConfig))
    {
        LOG_ERROR("Failed to initialize SRenderer");
        return;
    }

    m_renderer->setParticleSystem(m_particle.get());

    m_gameRunning = true;

    // Configure physics system
    m_physics->setGravity({gravity.x, gravity.y});  // Box2D uses Y-up convention
    // Use the stored member values so the members are read/used and
    // do not trigger "unused member" style warnings from static analyzers
    m_physics->setSubStepCount(m_subStepCount);  // Set substep count for stability
    m_physics->setTimeStep(m_timeStep);          // Set fixed timestep

    // Initialize input manager and register window event handling.
    // The engine does not initialize ImGui, so default to not forwarding
    // events to ImGui to avoid undefined behavior.
    m_input->initialize(m_renderer->getWindow(), false);
    m_input->subscribe(
        [this](const InputEvent& ev)
        {
            if (ev.type == InputEventType::WindowClosed)
            {
                auto* window = m_renderer->getWindow();
                if (window)
                {
                    window->close();
                }
                m_gameRunning = false;
            }
        });

    // Initialize audio system
    m_audio->initialize();

    // Initialize particle system with default scale
    // Note: Users can re-initialize with different scale if needed
    m_particle->initialize(m_renderer->getWindow(), pixelsPerMeter);

    // Maintain ordered list for per-frame updates (input -> scripts -> physics -> camera -> particle -> audio)
    // Audio is marked as PostFlush via ISystem::stage().
    m_systemOrder = {m_input.get(), m_script.get(), m_physics.get(), m_camera.get(), m_particle.get(), m_audio.get()};

    LOG_INFO("All core systems initialized");
}

GameEngine::~GameEngine()
{
    // Shutdown input manager
    if (m_input)
    {
        m_input->shutdown();
    }

    // Shutdown renderer
    if (m_renderer)
    {
        m_renderer->shutdown();
    }

    if (m_particle)
    {
        m_particle->shutdown();
    }

    if (m_audio)
    {
        m_audio->shutdown();
    }

    LOG_INFO("GameEngine shutting down");
    Logger::shutdown();
}

void GameEngine::readInputs()
{
    // Intentionally empty - SInput polls SFML events and dispatches them.
}

void GameEngine::update(float deltaTime)
{
    auto runStage = [this, deltaTime](Systems::UpdateStage stage)
    {
        for (auto* system : m_systemOrder)
        {
            if (!system || system->stage() != stage)
            {
                continue;
            }

            if (system->usesFixedTimestep())
            {
                m_accumulator += deltaTime;
                const float maxAccumulator = m_timeStep * 10.0f;  // Cap to prevent spiral of death
                if (m_accumulator > maxAccumulator)
                {
                    m_accumulator = maxAccumulator;
                }

                while (m_accumulator >= m_timeStep)
                {
                    system->fixedUpdate(m_timeStep, m_world);
                    m_accumulator -= m_timeStep;
                }
                continue;
            }

            system->update(deltaTime, m_world);
        }
    };

    runStage(Systems::UpdateStage::PreFlush);

    // Apply deferred structural commands after pre-flush systems have finished updating to avoid iterator invalidation
    m_world.flushCommandBuffer();

    runStage(Systems::UpdateStage::PostFlush);
}

void GameEngine::render()
{
    if (!m_renderer)
    {
        return;
    }

    m_renderer->clear(Color::Black);
    m_renderer->render(m_world);
    m_renderer->display();
}

bool GameEngine::is_running() const
{
    return m_gameRunning;
}
Systems::S2DPhysics& GameEngine::getPhysics()
{
    return *m_physics;
}

Systems::SInput& GameEngine::getInputManager()
{
    return *m_input;
}

Systems::SAudio& GameEngine::getAudioSystem()
{
    return *m_audio;
}

Systems::SRenderer& GameEngine::getRenderer()
{
    return *m_renderer;
}

Systems::SParticle& GameEngine::getParticleSystem()
{
    return *m_particle;
}

void GameEngine::registerComponentTypes()
{
    // Register stable component names for diagnostics and tooling
    using namespace Components;

    m_world.registerTypeName<CTransform>("CTransform");
    m_world.registerTypeName<CRenderable>("CRenderable");
    m_world.registerTypeName<CPhysicsBody2D>("CPhysicsBody2D");
    m_world.registerTypeName<CCollider2D>("CCollider2D");
    m_world.registerTypeName<CMaterial>("CMaterial");
    m_world.registerTypeName<CTexture>("CTexture");
    m_world.registerTypeName<CShader>("CShader");
    m_world.registerTypeName<CName>("CName");
    m_world.registerTypeName<CInputController>("CInputController");
    m_world.registerTypeName<CParticleEmitter>("CParticleEmitter");
    m_world.registerTypeName<CCamera>("CCamera");
    m_world.registerTypeName<CNativeScript>("CNativeScript");
    m_world.registerTypeName<CAudioSource>("CAudioSource");
    m_world.registerTypeName<CAudioListener>("CAudioListener");

    validateComponentTypeNames();
}

void GameEngine::validateComponentTypeNames()
{
    using namespace Components;

    auto validate = [this](auto typeTag, const char* stableName)
    {
        using T                      = decltype(typeTag);
        const std::string registered = m_world.getTypeName<T>();
        if (registered != stableName)
        {
            LOG_ERROR("Component type '{}' registered as '{}' but expected '{}'", typeid(T).name(), registered, stableName);
            return;
        }

        const std::type_index fromName = m_world.getTypeFromName(stableName);
        if (fromName != std::type_index(typeid(T)))
        {
            LOG_ERROR("Component lookup for '{}' returned different type index", stableName);
        }
    };

    validate(CTransform{}, "CTransform");
    validate(CRenderable{}, "CRenderable");
    validate(CPhysicsBody2D{}, "CPhysicsBody2D");
    validate(CCollider2D{}, "CCollider2D");
    validate(CMaterial{}, "CMaterial");
    validate(CTexture{}, "CTexture");
    validate(CShader{}, "CShader");
    validate(CName{}, "CName");
    validate(CInputController{}, "CInputController");
    validate(CParticleEmitter{}, "CParticleEmitter");
    validate(CCamera{}, "CCamera");
    validate(CNativeScript{}, "CNativeScript");
    validate(CAudioSource{}, "CAudioSource");
    validate(CAudioListener{}, "CAudioListener");
}
