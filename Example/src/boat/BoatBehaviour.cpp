#include "BoatBehaviour.h"

#include <algorithm>
#include <cmath>
#include <string>

#include <ActionBinding.h>
#include <Components.h>
#include <Logger.h>
#include <S2DPhysics.h>
#include <SAudio.h>
#include <SystemLocator.h>
#include <World.h>

#include <ObjectiveEvents.h>
#include <TriggerEvents.h>

namespace Example
{

static Entity findEntityByName(World& world, const std::string& name)
{
    Entity result = Entity::null();

    world.components().view<Components::CName>(
        [&](Entity entity, const Components::CName& cname)
        {
            if (cname.name == name)
            {
                result = entity;
            }
        });

    return result;
}

void BoatBehaviour::onCreate(Entity self, World& world)
{
    Components::CInputController* input = world.components().tryGet<Components::CInputController>(self);
    if (!input)
    {
        input = world.components().add<Components::CInputController>(self);
    }

    ensureEffectEntities(world);

    bindMovement(*input);

    setupFixedMovement(self, world);

    // Count barrel overlaps via sensor events and complete the demo objective at 10.
    m_triggerEnterSub = ScopedSubscription(world.events(),
                                           world.events().subscribe<Physics::TriggerEnter>(
                                               [this](const Physics::TriggerEnter& ev, World& w)
                                               {
                                                   if (!m_barrelSensor.isValid() || ev.triggerEntity != m_barrelSensor)
                                                   {
                                                       return;
                                                   }

                                                   const auto* otherName = w.get<Components::CName>(ev.otherEntity);
                                                   if (!otherName || otherName->name != "Barrel")
                                                   {
                                                       return;
                                                   }

                                                   ++m_barrelsHitCount;
                                                   w.events().emit(Objectives::ObjectiveSignal{
                                                       std::string("example.signal.boat.hit_10_barrels")});
                                               }));

    LOG_INFO_CONSOLE("Controls:");
    LOG_INFO_CONSOLE("  WASD : Move player boat (W/S = forward/back, A/D = rotate)");
}

void BoatBehaviour::onUpdate(float /*deltaTime*/, Entity self, World& world)
{
    auto components = world.components();

    Components::CInputController* input = components.tryGet<Components::CInputController>(self);
    if (!input)
    {
        return;
    }

    ensureEffectEntities(world);

    Systems::SAudio& audio = Systems::SystemLocator::audio();

    const bool forward  = input->isActionActive("MoveForward");
    const bool backward = input->isActionActive("MoveBackward");
    const bool left     = input->isActionActive("RotateLeft");
    const bool right    = input->isActionActive("RotateRight");

    // --- Objectives demo: emit signals once when controls are used ---
    if (forward && !m_sentMoveForwardSignal)
    {
        m_sentMoveForwardSignal = true;
        world.events().emit(Objectives::ObjectiveSignal{std::string("example.signal.boat.move_forward")});
    }

    if ((left || right) && !m_sentRotateSignal)
    {
        m_sentRotateSignal = true;
        world.events().emit(Objectives::ObjectiveSignal{std::string("example.signal.boat.rotate")});
    }

    if (backward && !m_sentReverseSignal)
    {
        m_sentReverseSignal = true;
        world.events().emit(Objectives::ObjectiveSignal{std::string("example.signal.boat.reverse")});
    }

    if (forward || backward)
    {
        if (!m_motorPlaying)
        {
            m_motorPlaying = audio.playSfx(self, "motor_boat", true, m_motorVolume);
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

    if (!m_hullSpray.isValid())
    {
        return;
    }

    // Hull spray emission rate scales with boat speed.
    const b2Vec2 vel   = Systems::SystemLocator::physics().getLinearVelocity(self);
    const float  speed = std::sqrt((vel.x * vel.x) + (vel.y * vel.y));

    Components::CParticleEmitter* emitter = components.tryGet<Components::CParticleEmitter>(m_hullSpray);
    if (!emitter)
    {
        return;
    }

    const float minSpeed     = m_minSpeedForSpray;
    const float maxSpeed     = m_maxSpeedForSpray;
    const float maxEmission  = m_maxEmissionRate;
    const float minPartSpeed = m_sprayMinParticleSpeed;
    const float maxPartSpeed = m_sprayMaxParticleSpeed;
    const float boostFactor  = m_spraySpeedBoostFactor;

    float emissionRate = 0.0f;
    if (speed > minSpeed && maxSpeed > minSpeed)
    {
        float normalizedSpeed = (speed - minSpeed) / (maxSpeed - minSpeed);
        normalizedSpeed       = std::clamp(normalizedSpeed, 0.0f, 1.0f);
        emissionRate          = maxEmission * (normalizedSpeed * normalizedSpeed);

        const float speedMultiplier = 1.0f + (speed / maxSpeed) * boostFactor;
        emitter->setMinSpeed(minPartSpeed * speedMultiplier);
        emitter->setMaxSpeed(maxPartSpeed * speedMultiplier);
    }

    emitter->setEmissionRate(emissionRate);
}

void BoatBehaviour::ensureEffectEntities(World& world)
{
    if (!m_bubbleTrail.isValid())
    {
        m_bubbleTrail = findEntityByName(world, m_bubbleTrailName);
    }
    if (!m_hullSpray.isValid())
    {
        m_hullSpray = findEntityByName(world, m_hullSprayName);
    }
    if (!m_barrelSensor.isValid())
    {
        m_barrelSensor = findEntityByName(world, m_barrelSensorName);
    }
}

void BoatBehaviour::setupFixedMovement(Entity self, World& world)
{
    World*               worldPtr      = &world;
    Systems::S2DPhysics* physicsSystem = &Systems::SystemLocator::physics();

    physicsSystem->setFixedUpdateCallback(
        self,
        [this, self, worldPtr, physicsSystem](float /*timeStep*/)
        {
            if (!worldPtr)
            {
                return;
            }

            auto components = worldPtr->components();

            const Components::CInputController* input = components.tryGet<Components::CInputController>(self);
            if (!input)
            {
                return;
            }

            ensureEffectEntities(*worldPtr);

            const bool forward  = input->isActionActive("MoveForward");
            const bool backward = input->isActionActive("MoveBackward");
            const bool left     = input->isActionActive("RotateLeft");
            const bool right    = input->isActionActive("RotateRight");

            const float playerForce        = m_playerForce;
            const float playerTurningForce = m_playerTurningForce;

            // --- Thrust (frame-rate independent; runs at fixed 60Hz) ---
            if (forward)
            {
                const b2Vec2 f = physicsSystem->getForwardVector(self);
                physicsSystem->applyForceToCenter(self, b2Vec2{f.x * playerForce, f.y * playerForce});
            }
            else if (backward)
            {
                const b2Vec2 f = physicsSystem->getForwardVector(self);
                physicsSystem->applyForceToCenter(self, b2Vec2{-f.x * (playerForce * 0.5f), -f.y * (playerForce * 0.5f)});
            }

            // --- Rudder steering ---
            if (left || right)
            {
                const float rudderOffsetMeters      = m_rudderOffsetMeters;
                const float rudderForceMultiplier   = m_rudderForceMultiplier;
                const float rudderSmoothK           = m_rudderSmoothK;
                const float minSpeedForSteering     = m_minSpeedForSteering;
                const float rudderMinEffectiveScale = m_rudderMinEffectiveScale;

                const b2Vec2 f   = physicsSystem->getForwardVector(self);
                const b2Vec2 r   = physicsSystem->getRightVector(self);
                const b2Vec2 vel = physicsSystem->getLinearVelocity(self);

                const float forwardVelSigned = (f.x * vel.x) + (f.y * vel.y);
                const float absForwardVel    = std::fabs(forwardVelSigned);

                if (absForwardVel >= minSpeedForSteering)
                {
                    const b2Vec2 pos = physicsSystem->getPosition(self);
                    const b2Vec2 stern{pos.x - f.x * rudderOffsetMeters, pos.y - f.y * rudderOffsetMeters};

                    const float speedEffective = std::max(0.0f, absForwardVel - minSpeedForSteering);
                    const float normalized     = speedEffective / (speedEffective + rudderSmoothK);
                    const float speedFactor = rudderMinEffectiveScale + normalized * (1.0f - rudderMinEffectiveScale);

                    const float forceMag = playerTurningForce * rudderForceMultiplier * speedFactor;

                    b2Vec2 lateral{0.0f, 0.0f};
                    if (left)
                    {
                        lateral = (forwardVelSigned >= 0.0f) ? r : b2Vec2{-r.x, -r.y};
                    }
                    else
                    {
                        lateral = (forwardVelSigned >= 0.0f) ? b2Vec2{-r.x, -r.y} : r;
                    }

                    physicsSystem->applyForce(self, b2Vec2{lateral.x * forceMag, lateral.y * forceMag}, stern);
                }
            }

            // --- Keep particle emitters pinned to the boat transform ---
            const Components::CTransform* boatT = components.tryGet<Components::CTransform>(self);
            if (!boatT)
            {
                return;
            }

            if (m_bubbleTrail.isValid())
            {
                if (Components::CTransform* t = components.tryGet<Components::CTransform>(m_bubbleTrail))
                {
                    t->setPosition(boatT->getPosition());
                    t->setRotation(boatT->getRotation());
                }
            }
            if (m_hullSpray.isValid())
            {
                if (Components::CTransform* t = components.tryGet<Components::CTransform>(m_hullSpray))
                {
                    t->setPosition(boatT->getPosition());
                    t->setRotation(boatT->getRotation());
                }
            }

            if (m_barrelSensor.isValid())
            {
                if (Components::CTransform* t = components.tryGet<Components::CTransform>(m_barrelSensor))
                {
                    t->setPosition(boatT->getPosition());
                    t->setRotation(boatT->getRotation());
                }
            }
        });
}

void BoatBehaviour::bindMovement(Components::CInputController& input)
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
