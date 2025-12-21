#pragma once

#include <Components.h>

#include <string>

namespace Components
{
struct CInputController;
}

namespace Example
{

class BoatBehaviour final : public Components::INativeScript
{
public:
    // Keep stable name as "Boat" so existing saves still load.
    static constexpr const char* kScriptName = "Boat";

    void onCreate(Entity self, World& world) override;
    void onUpdate(float deltaTime, Entity self, World& world) override;

private:
    static void bindMovement(Components::CInputController& input);

    void setupFixedMovement(Entity self, World& world);

    void ensureEffectEntities(World& world);

    bool m_motorPlaying = false;

    // --- Behaviour tuning ---
    float m_playerForce             = 5.0f;
    float m_playerTurningForce      = 0.5f;
    float m_motorVolume             = 0.45f;
    float m_rudderOffsetMeters      = 0.35f;
    float m_rudderForceMultiplier   = 1.0f;
    float m_rudderSmoothK           = 0.18f;
    float m_minSpeedForSteering     = 0.15f;
    float m_rudderMinEffectiveScale = 0.025f;
    float m_minSpeedForSpray        = 0.05f;
    float m_maxSpeedForSpray        = 1.5f;
    float m_maxEmissionRate         = 5000.0f;
    float m_sprayMinParticleSpeed   = 0.1f;
    float m_sprayMaxParticleSpeed   = 0.4f;
    float m_spraySpeedBoostFactor   = 0.5f;

    // --- Authored effect entity names (CName) ---
    std::string m_bubbleTrailName = "BoatBubbleTrail";
    std::string m_hullSprayName   = "BoatHullSpray";

    // --- Cached effect entities ---
    Entity m_bubbleTrail = Entity::null();
    Entity m_hullSpray   = Entity::null();
};

}  // namespace Example
