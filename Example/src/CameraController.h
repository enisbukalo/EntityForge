#pragma once

#include <string>

#include <Components.h>

namespace Components
{
struct CInputController;
}

namespace Example
{

// Creates a camera controller entity that handles zoom, rotation, and camera switching.
// The controller will manipulate the camera with the given name.
Entity spawnCameraController(World& world, std::string_view targetCameraName);

class CameraController final : public Components::INativeScript
{
public:
    explicit CameraController(std::string_view targetCameraName);
    ~CameraController() override;

    void onCreate(Entity self, World& world) override;
    void onUpdate(float deltaTime, Entity self, World& world) override;

private:
    static void bindCameraActions(Components::CInputController& input);

    // Returns the entity with a CCamera matching m_targetCameraName, or null if not found.
    Entity findCameraEntity(World& world) const;

    // Collects all enabled camera names in sorted order for deterministic switching.
    std::vector<std::string> collectEnabledCameraNames(World& world) const;

    // Configuration
    std::string m_targetCameraName;
    float       m_rotateSpeed = 1.0f;     // radians per second
    float       m_zoomSpeed   = 0.3f;     // zoom units per second
    float       m_scrollZoomSpeed = 0.05f; // zoom units per scroll tick

    // Hard zoom limits (enforced regardless of user config)
    static constexpr float kMinZoom = 0.5f;
    static constexpr float kMaxZoom = 10.0f;

    // Scroll wheel state
    float  m_scrollDelta   = 0.0f;
    size_t m_subscriberId  = 0;
};

}  // namespace Example
