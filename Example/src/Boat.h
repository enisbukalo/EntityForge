#pragma once

#include <Components.h>

namespace Components
{
struct CInputController;
}

namespace Example
{

// Creates the boat entity with its default components (transform, sprite, physics body, collider, script).
// Scene composition code (main) should call this rather than hardcoding boat-specific data.
Entity spawnBoat(World& world);

class Boat final : public Components::INativeScript
{
public:
    void onCreate(Entity self, World& world) override;
    void onUpdate(float deltaTime, Entity self, World& world) override;

private:
    static void bindMovement(Components::CInputController& input);

    void setupParticles(Entity self, World& world);
    void setupFixedMovement(Entity self, World& world);

    // Tuned to match the old Example feel (pre-refactor).
    static constexpr float kPlayerForce        = 5.0f;
    static constexpr float kPlayerTurningForce = 0.5f;
    static constexpr float kMotorVolume        = 0.45f;

    static constexpr float kRudderOffsetMeters      = 0.35f;
    static constexpr float kRudderForceMultiplier   = 1.0f;
    static constexpr float kRudderSmoothK           = 0.18f;
    static constexpr float kMinSpeedForSteering     = 0.15f;
    static constexpr float kRudderMinEffectiveScale = 0.025f;

    bool m_motorPlaying = false;

    Entity m_bubbleTrail = Entity::null();
    Entity m_hullSpray   = Entity::null();
};

}  // namespace Example
