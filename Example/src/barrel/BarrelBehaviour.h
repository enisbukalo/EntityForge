#pragma once

#include <Components.h>

namespace Example
{

class BarrelBehaviour final : public Components::INativeScript
{
public:
    static constexpr const char* kScriptName = "BarrelScript";

    void onCreate(Entity self, World& world) override;
    void onUpdate(float deltaTime, Entity self, World& world) override;

private:
    float m_minSpeedForSpray = 0.05f;
    float m_maxSpeedForSpray = 0.75f;
    float m_maxEmissionRate  = 1250.0f;

    float m_emitterMinSpeed = 0.05f;
    float m_emitterMaxSpeed = 0.75f;
};

}  // namespace Example
