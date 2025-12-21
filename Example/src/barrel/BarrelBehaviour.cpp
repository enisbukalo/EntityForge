#include "BarrelBehaviour.h"

#include <algorithm>
#include <cmath>

#include <Components.h>
#include <S2DPhysics.h>
#include <SystemLocator.h>
#include <World.h>

namespace Example
{

void BarrelBehaviour::onCreate(Entity /*self*/, World& /*world*/) {}

void BarrelBehaviour::onUpdate(float /*deltaTime*/, Entity self, World& world)
{
    Components::CParticleEmitter* emitter = world.components().tryGet<Components::CParticleEmitter>(self);
    if (!emitter)
    {
        return;
    }

    const b2Vec2 vel   = Systems::SystemLocator::physics().getLinearVelocity(self);
    const float  speed = std::sqrt((vel.x * vel.x) + (vel.y * vel.y));

    float emissionRate = 0.0f;
    if (speed > m_minSpeedForSpray && m_maxSpeedForSpray > m_minSpeedForSpray)
    {
        float normalizedSpeed = (speed - m_minSpeedForSpray) / (m_maxSpeedForSpray - m_minSpeedForSpray);
        normalizedSpeed       = std::clamp(normalizedSpeed, 0.0f, 1.0f);
        emissionRate          = m_maxEmissionRate * (normalizedSpeed * normalizedSpeed);

        const float speedMultiplier = m_emitterMaxSpeed + (normalizedSpeed * m_emitterMaxSpeed);
        emitter->setMinSpeed(m_emitterMinSpeed * speedMultiplier);
        emitter->setMaxSpeed(m_emitterMaxSpeed * speedMultiplier);
    }

    emitter->setEmissionRate(emissionRate);
}

}  // namespace Example
