#pragma once

#include <string>
#include <vector>

#include <Components.h>
#include <ISerializableScript.h>

namespace Components
{
struct CInputController;
}

namespace Example
{

class CameraControllerBehaviour final : public Components::INativeScript, public Components::ISerializableScript
{
public:
    static constexpr const char* kScriptName = "CameraController";

    CameraControllerBehaviour();
    explicit CameraControllerBehaviour(std::string_view targetCameraName);
    ~CameraControllerBehaviour() override;

    void onCreate(Entity self, World& world) override;
    void onUpdate(float deltaTime, Entity self, World& world) override;

    void serializeFields(Serialization::ScriptFieldWriter& out) const override;
    void deserializeFields(const Serialization::ScriptFieldReader& in) override;

private:
    static void bindCameraActions(Components::CInputController& input);

    Entity                   findCameraEntity(World& world) const;
    std::vector<std::string> collectEnabledCameraNames(World& world) const;

    std::string m_targetCameraName;
    float       m_rotateSpeed     = 1.0f;
    float       m_zoomSpeed       = 0.3f;
    float       m_scrollZoomSpeed = 0.05f;

    static constexpr float kMinZoom = 0.5f;
    static constexpr float kMaxZoom = 10.0f;

    float  m_scrollDelta  = 0.0f;
    size_t m_subscriberId = 0;
};

}  // namespace Example
