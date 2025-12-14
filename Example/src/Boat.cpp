#include "Boat.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include <ActionBinding.h>
#include <Color.h>
#include <Components.h>
#include <Logger.h>
#include <S2DPhysics.h>
#include <SAudio.h>
#include <SystemLocator.h>
#include <World.h>

namespace Example
{

Entity spawnBoat(World& world)
{
    // Tuned values copied from the Example on `master`.
    static constexpr float kBoatPosX = 9.20209f;
    static constexpr float kBoatPosY = 7.90827f;
    static constexpr float kBoatRot  = 1.73084f;

    static constexpr float kBoatLinearDamping  = 0.75f;
    static constexpr float kBoatAngularDamping = 0.75f;
    static constexpr float kBoatGravityScale   = 1.0f;

    static constexpr float kBoatColliderDensity     = 5.0f;
    static constexpr float kBoatColliderFriction    = 0.5f;
    static constexpr float kBoatColliderRestitution = 0.125f;

    static const std::vector<std::vector<Vec2>> kBoatHullFixtures = {
        {{0.225f, 0.0f}, {-0.225f, 0.0f}, {-0.225f, -0.0875f}, {-0.1575f, -0.39375f}, {0.1575f, -0.39375f}, {0.225f, -0.0875f}},
        {{-0.225f, 0.0f}, {0.225f, 0.0f}, {0.223438f, 0.0401042f}, {-0.223438f, 0.0401042f}},
        {{-0.223438f, 0.0401042f}, {0.223438f, 0.0401042f}, {0.21875f, 0.0802083f}, {-0.21875f, 0.0802083f}},
        {{-0.21875f, 0.0802083f}, {0.21875f, 0.0802083f}, {0.210938f, 0.120313f}, {-0.210938f, 0.120313f}},
        {{-0.210938f, 0.120313f}, {0.210938f, 0.120313f}, {0.2f, 0.160417f}, {-0.2f, 0.160417f}},
        {{-0.2f, 0.160417f}, {0.2f, 0.160417f}, {0.185937f, 0.200521f}, {-0.185937f, 0.200521f}},
        {{-0.185937f, 0.200521f}, {0.185937f, 0.200521f}, {0.16875f, 0.240625f}, {-0.16875f, 0.240625f}},
        {{-0.16875f, 0.240625f}, {0.16875f, 0.240625f}, {0.148438f, 0.280729f}, {-0.148438f, 0.280729f}},
        {{-0.148438f, 0.280729f}, {0.148438f, 0.280729f}, {0.125f, 0.320833f}, {-0.125f, 0.320833f}},
        {{-0.125f, 0.320833f}, {0.125f, 0.320833f}, {0.0984375f, 0.360938f}, {-0.0984375f, 0.360938f}},
        {{-0.0984375f, 0.360938f}, {0.0984375f, 0.360938f}, {0.06875f, 0.401042f}, {-0.06875f, 0.401042f}},
        {{-0.06875f, 0.401042f}, {0.06875f, 0.401042f}, {0.0359375f, 0.441146f}, {-0.0359375f, 0.441146f}},
        {{-0.0359375f, 0.441146f}, {0.0359375f, 0.441146f}, {0.0f, 0.48125f}},
    };

    Entity boat = world.createEntity();

    world.components().add<Components::CTransform>(boat, Vec2{kBoatPosX, kBoatPosY}, Vec2{1.0f, 1.0f}, kBoatRot);
    world.components().add<Components::CTexture>(boat, "assets/textures/boat.png");
    world.components().add<Components::CRenderable>(boat, Components::VisualType::Sprite, Color::White, 10, true);
    world.components().add<Components::CMaterial>(boat, Color::White, Components::BlendMode::Alpha, 1.0f);

    auto* body           = world.components().add<Components::CPhysicsBody2D>(boat);
    body->bodyType       = Components::BodyType::Dynamic;
    body->fixedRotation  = false;
    body->linearDamping  = kBoatLinearDamping;
    body->angularDamping = kBoatAngularDamping;
    body->gravityScale   = kBoatGravityScale;

    auto* collider        = world.components().add<Components::CCollider2D>(boat);
    collider->sensor      = false;
    collider->density     = kBoatColliderDensity;
    collider->friction    = kBoatColliderFriction;
    collider->restitution = kBoatColliderRestitution;
    if (!kBoatHullFixtures.empty())
    {
        collider->createPolygon(kBoatHullFixtures.front(), 0.02f);
        for (size_t i = 1; i < kBoatHullFixtures.size(); ++i)
        {
            collider->addPolygon(kBoatHullFixtures[i], 0.02f);
        }
    }

    world.components().add<Components::CInputController>(boat);

    auto* script = world.components().add<Components::CNativeScript>(boat);
    script->bind<Example::Boat>();

    return boat;
}

namespace
{
Components::CParticleEmitter makeBubbleTrailEmitter()
{
    Components::CParticleEmitter e;
    e.setDirection(Vec2(0.0f, -1.0f));
    e.setSpreadAngle(1.2f);
    e.setMinSpeed(0.05f);
    e.setMaxSpeed(0.2f);
    e.setMinLifetime(3.0f);
    e.setMaxLifetime(3.0f);
    e.setMinSize(0.005f);
    e.setMaxSize(0.025f);
    e.setEmissionRate(300.0f);
    e.setStartColor(Color::White);
    e.setEndColor(Color::White);
    e.setStartAlpha(1.0f);
    e.setEndAlpha(0.5f);
    e.setGravity(Vec2(0.0f, 0.0f));
    e.setFadeOut(true);
    // On `master`, the emitter defaults shrink=true; keep that behavior.
    e.setShrink(true);
    e.setShrinkEndScale(0.1f);
    e.setMaxParticles(1000);
    e.setZIndex(5);
    e.setPositionOffset(Vec2(0.0f, -0.65625f));
    e.setEmissionShape(Components::EmissionShape::Point);
    e.setLineStart(Vec2(-0.5f, 0.0f));
    e.setLineEnd(Vec2(0.5f, 0.0f));
    e.setEmitFromEdge(true);
    e.setEmitOutward(false);
    e.setTexturePath("assets/textures/bubble.png");
    return e;
}

Components::CParticleEmitter makeHullSprayEmitter()
{
    Components::CParticleEmitter e;
    e.setDirection(Vec2(0.0f, 1.0f));
    e.setSpreadAngle(0.4f);
    e.setMinSpeed(0.122925f);
    e.setMaxSpeed(0.4917f);
    e.setMinLifetime(0.5f);
    e.setMaxLifetime(2.4f);
    e.setMinSize(0.006f);
    e.setMaxSize(0.02f);
    e.setEmissionRate(938.808f);
    e.setStartColor(Color(220, 240, 255, 255));
    e.setEndColor(Color(255, 255, 255, 255));
    e.setStartAlpha(0.9f);
    e.setEndAlpha(0.0f);
    e.setGravity(Vec2(0.0f, 0.0f));
    e.setMinRotationSpeed(-3.0f);
    e.setMaxRotationSpeed(3.0f);
    e.setFadeOut(true);
    e.setShrink(true);
    e.setShrinkEndScale(0.1f);
    e.setMaxParticles(7500);
    e.setZIndex(5);
    e.setEmissionShape(Components::EmissionShape::Polygon);
    e.setEmitFromEdge(true);
    e.setEmitOutward(true);
    e.setLineStart(Vec2(-0.5f, 0.0f));
    e.setLineEnd(Vec2(0.5f, 0.0f));
    e.setTexturePath("assets/textures/bubble.png");

    // Boat hull spray polygon (ported from the old Example constants).
    e.setPolygonVertices({
        {-0.1575f, -0.39375f},    {0.1575f, -0.39375f},    {0.225f, -0.0875f},       {0.225f, 0.0f},
        {0.223438f, 0.0401042f},  {0.21875f, 0.0802083f},  {0.210938f, 0.120313f},   {0.2f, 0.160417f},
        {0.185937f, 0.200521f},   {0.16875f, 0.240625f},   {0.148438f, 0.280729f},   {0.125f, 0.320833f},
        {0.0984375f, 0.360938f},  {0.06875f, 0.401042f},   {0.0359375f, 0.441146f},  {0.0f, 0.48125f},
        {-0.0359375f, 0.441146f}, {-0.06875f, 0.401042f},  {-0.0984375f, 0.360938f}, {-0.125f, 0.320833f},
        {-0.148438f, 0.280729f},  {-0.16875f, 0.240625f},  {-0.185937f, 0.200521f},  {-0.2f, 0.160417f},
        {-0.210938f, 0.120313f},  {-0.21875f, 0.0802083f}, {-0.223438f, 0.0401042f}, {-0.225f, 0.0f},
        {-0.225f, -0.0875f},
    });

    return e;
}

}  // namespace

void Boat::onCreate(Entity self, World& world)
{
    auto* input = world.components().tryGet<Components::CInputController>(self);
    if (!input)
    {
        input = world.components().add<Components::CInputController>(self);
    }

    bindMovement(*input);

    setupParticles(self, world);
    setupFixedMovement(self, world);

    LOG_INFO_CONSOLE("Controls:");
    LOG_INFO_CONSOLE("  WASD : Move player boat (W/S = forward/back, A/D = rotate)");
}

void Boat::onUpdate(float /*deltaTime*/, Entity self, World& world)
{
    auto* input = world.components().tryGet<Components::CInputController>(self);
    if (!input)
    {
        return;
    }

    auto& audio = Systems::SystemLocator::audio();

    const bool forward  = input->isActionActive("MoveForward");
    const bool backward = input->isActionActive("MoveBackward");

    if (forward || backward)
    {
        if (!m_motorPlaying)
        {
            m_motorPlaying = audio.playSfx(self, "motor_boat", true, kMotorVolume);
        }
    }
    else
    {
        if (m_motorPlaying)
        {
            audio.stopSfx(self);
            m_motorPlaying = false;
        }
    }

    // Match `master`: hull spray emission rate scales with boat speed.
    const b2Vec2 vel   = Systems::SystemLocator::physics().getLinearVelocity(self);
    const float  speed = std::sqrt((vel.x * vel.x) + (vel.y * vel.y));

    if (m_hullSpray.isValid())
    {
        auto* emitter = world.components().tryGet<Components::CParticleEmitter>(m_hullSpray);
        if (emitter)
        {
            const float MIN_SPEED_FOR_SPRAY = 0.05f;
            const float MAX_SPEED_FOR_SPRAY = 2.0f;
            const float MIN_EMISSION_RATE   = 0.0f;
            const float MAX_EMISSION_RATE   = 5000.0f;

            float emissionRate = 0.0f;
            if (speed > MIN_SPEED_FOR_SPRAY)
            {
                float normalizedSpeed = (speed - MIN_SPEED_FOR_SPRAY) / (MAX_SPEED_FOR_SPRAY - MIN_SPEED_FOR_SPRAY);
                normalizedSpeed       = std::min(1.0f, std::max(0.0f, normalizedSpeed));
                emissionRate = MIN_EMISSION_RATE + (MAX_EMISSION_RATE - MIN_EMISSION_RATE) * (normalizedSpeed * normalizedSpeed);

                float speedMultiplier = 1.0f + (speed / MAX_SPEED_FOR_SPRAY) * 0.5f;
                emitter->setMinSpeed(0.1f * speedMultiplier);
                emitter->setMaxSpeed(0.4f * speedMultiplier);
            }

            emitter->setEmissionRate(emissionRate);
        }
    }
}

void Boat::setupParticles(Entity self, World& world)
{
    auto*       boatTransform = world.components().tryGet<Components::CTransform>(self);
    const Vec2  pos           = boatTransform ? boatTransform->getPosition() : Vec2{0.0f, 0.0f};
    const float rotation      = boatTransform ? boatTransform->getRotation() : 0.0f;

    // Spawn separate entities for each emitter (only one CParticleEmitter per entity type).
    if (!m_bubbleTrail.isValid())
    {
        m_bubbleTrail = world.queueSpawn(Components::CTransform(pos, Vec2{1.0f, 1.0f}, rotation), makeBubbleTrailEmitter());
    }

    if (!m_hullSpray.isValid())
    {
        m_hullSpray = world.queueSpawn(Components::CTransform(pos, Vec2{1.0f, 1.0f}, rotation), makeHullSprayEmitter());
    }
}

void Boat::setupFixedMovement(Entity self, World& world)
{
    World* worldPtr = &world;
    auto&  physics  = Systems::SystemLocator::physics();

    physics.setFixedUpdateCallback(
        self,
        [this, self, worldPtr](float /*timeStep*/)
        {
            if (!worldPtr)
            {
                return;
            }

            auto  components = worldPtr->components();
            auto* input      = components.tryGet<Components::CInputController>(self);
            if (!input)
            {
                return;
            }

            const bool forward  = input->isActionActive("MoveForward");
            const bool backward = input->isActionActive("MoveBackward");
            const bool left     = input->isActionActive("RotateLeft");
            const bool right    = input->isActionActive("RotateRight");

            auto& physics = Systems::SystemLocator::physics();

            // --- Thrust (frame-rate independent; runs at fixed 60Hz) ---
            if (forward)
            {
                const b2Vec2 f = physics.getForwardVector(self);
                physics.applyForceToCenter(self, b2Vec2{f.x * kPlayerForce, f.y * kPlayerForce});
            }
            else if (backward)
            {
                const b2Vec2 f = physics.getForwardVector(self);
                physics.applyForceToCenter(self, b2Vec2{-f.x * (kPlayerForce * 0.5f), -f.y * (kPlayerForce * 0.5f)});
            }

            // --- Rudder steering (matches old behavior: steer only when moving) ---
            if (left || right)
            {
                const b2Vec2 f   = physics.getForwardVector(self);
                const b2Vec2 r   = physics.getRightVector(self);
                const b2Vec2 vel = physics.getLinearVelocity(self);

                const float forwardVelSigned = (f.x * vel.x) + (f.y * vel.y);
                const float absForwardVel    = std::fabs(forwardVelSigned);

                if (absForwardVel >= kMinSpeedForSteering)
                {
                    const b2Vec2 pos = physics.getPosition(self);
                    const b2Vec2 stern{pos.x - f.x * kRudderOffsetMeters, pos.y - f.y * kRudderOffsetMeters};

                    const float speedEffective = std::max(0.0f, absForwardVel - kMinSpeedForSteering);
                    const float normalized     = speedEffective / (speedEffective + kRudderSmoothK);
                    const float speedFactor = kRudderMinEffectiveScale + normalized * (1.0f - kRudderMinEffectiveScale);

                    const float forceMag = kPlayerTurningForce * kRudderForceMultiplier * speedFactor;

                    // Choose lateral direction based on desired turn AND whether we're moving forward/backward.
                    b2Vec2 lateral{0.0f, 0.0f};
                    if (left)
                    {
                        lateral = (forwardVelSigned >= 0.0f) ? r : b2Vec2{-r.x, -r.y};
                    }
                    else
                    {
                        lateral = (forwardVelSigned >= 0.0f) ? b2Vec2{-r.x, -r.y} : r;
                    }

                    physics.applyForce(self, b2Vec2{lateral.x * forceMag, lateral.y * forceMag}, stern);
                }
            }

            // --- Keep particle emitters pinned to the boat transform ---
            auto* boatT = components.tryGet<Components::CTransform>(self);
            if (!boatT)
            {
                return;
            }

            if (m_bubbleTrail.isValid())
            {
                if (auto* t = components.tryGet<Components::CTransform>(m_bubbleTrail))
                {
                    t->setPosition(boatT->getPosition());
                    t->setRotation(boatT->getRotation());
                }
            }
            if (m_hullSpray.isValid())
            {
                if (auto* t = components.tryGet<Components::CTransform>(m_hullSpray))
                {
                    t->setPosition(boatT->getPosition());
                    t->setRotation(boatT->getRotation());
                }
            }
        });
}

void Boat::bindMovement(Components::CInputController& input)
{
    {
        ActionBinding b;
        b.keys    = {KeyCode::W};
        b.trigger = ActionTrigger::Held;
        input.bindings["MoveForward"].push_back({b, 0});
    }
    {
        ActionBinding b;
        b.keys    = {KeyCode::S};
        b.trigger = ActionTrigger::Held;
        input.bindings["MoveBackward"].push_back({b, 0});
    }
    {
        ActionBinding b;
        b.keys    = {KeyCode::A};
        b.trigger = ActionTrigger::Held;
        input.bindings["RotateLeft"].push_back({b, 0});
    }
    {
        ActionBinding b;
        b.keys    = {KeyCode::D};
        b.trigger = ActionTrigger::Held;
        input.bindings["RotateRight"].push_back({b, 0});
    }
}

}  // namespace Example
