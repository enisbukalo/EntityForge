#include "CameraControllerBehaviour.h"

#include <algorithm>
#include <cmath>

#include <ActionBinding.h>
#include <Components.h>
#include <InputEvents.h>
#include <Logger.h>
#include <SCamera.h>
#include <SInput.h>
#include <SystemLocator.h>
#include <World.h>

namespace Example
{

CameraControllerBehaviour::CameraControllerBehaviour() : m_targetCameraName("Main") {}

CameraControllerBehaviour::CameraControllerBehaviour(std::string_view targetCameraName) : m_targetCameraName(targetCameraName) {}

void CameraControllerBehaviour::onCreate(Entity self, World& world)
{
    Components::CInputController* input = world.components().tryGet<Components::CInputController>(self);
    if (!input)
    {
        input = world.components().add<Components::CInputController>(self);
    }
    bindCameraActions(*input);

    m_subscriberId = Systems::SystemLocator::input().subscribe(
        [this](const InputEvent& event)
        {
            if (event.type == InputEventType::MouseWheel)
            {
                m_scrollDelta += event.wheel.delta;
            }
        });

    LOG_INFO_CONSOLE("CameraController: Created, targeting camera '{}'", m_targetCameraName);
}

void CameraControllerBehaviour::onUpdate(float deltaTime, Entity self, World& world)
{
    Components::CInputController* input = world.components().tryGet<Components::CInputController>(self);
    if (!input)
    {
        return;
    }

    Entity cameraEntity = findCameraEntity(world);
    if (!cameraEntity.isValid())
    {
        return;
    }

    Components::CCamera* camera = world.components().tryGet<Components::CCamera>(cameraEntity);
    if (!camera)
    {
        return;
    }

    float zoomChange = 0.0f;

    if (input->isActionActive("CameraZoomIn"))
    {
        zoomChange -= m_zoomSpeed * deltaTime;
    }
    if (input->isActionActive("CameraZoomOut"))
    {
        zoomChange += m_zoomSpeed * deltaTime;
    }

    zoomChange += m_scrollDelta * m_scrollZoomSpeed;
    m_scrollDelta = 0.0f;

    camera->zoom += zoomChange;
    camera->zoom = std::clamp(camera->zoom, kMinZoom, kMaxZoom);

    float rotateChange = 0.0f;

    if (input->isActionActive("CameraRotateLeft"))
    {
        rotateChange -= m_rotateSpeed * deltaTime;
    }
    if (input->isActionActive("CameraRotateRight"))
    {
        rotateChange += m_rotateSpeed * deltaTime;
    }

    camera->rotationRadians += rotateChange;

    constexpr float kTwoPi = 2.0f * 3.14159265358979323846f;
    while (camera->rotationRadians < 0.0f)
    {
        camera->rotationRadians += kTwoPi;
    }
    while (camera->rotationRadians >= kTwoPi)
    {
        camera->rotationRadians -= kTwoPi;
    }

    const bool nextPressed = input->actionStates.count("CameraNext") && input->actionStates.at("CameraNext") == ActionState::Pressed;
    const bool prevPressed = input->actionStates.count("CameraPrev") && input->actionStates.at("CameraPrev") == ActionState::Pressed;

    if (nextPressed || prevPressed)
    {
        auto cameraNames = collectEnabledCameraNames(world);
        if (cameraNames.size() > 1)
        {
            auto it = std::find(cameraNames.begin(), cameraNames.end(), m_targetCameraName);
            if (it != cameraNames.end())
            {
                size_t currentIdx = static_cast<size_t>(std::distance(cameraNames.begin(), it));
                size_t newIdx     = currentIdx;

                if (nextPressed)
                {
                    newIdx = (currentIdx + 1) % cameraNames.size();
                }
                else if (prevPressed)
                {
                    newIdx = (currentIdx == 0) ? cameraNames.size() - 1 : currentIdx - 1;
                }

                m_targetCameraName = cameraNames[newIdx];
                Systems::SystemLocator::camera().setActiveCamera(m_targetCameraName);
                LOG_INFO_CONSOLE("CameraController: Switched to camera '{}'", m_targetCameraName);
            }
        }
    }
}

CameraControllerBehaviour::~CameraControllerBehaviour()
{
    if (m_subscriberId != 0)
    {
        Systems::SystemLocator::input().unsubscribe(m_subscriberId);
        m_subscriberId = 0;
    }
}

void CameraControllerBehaviour::serializeFields(Serialization::ScriptFieldWriter& out) const
{
    out.setString("targetCameraName", m_targetCameraName);
}

void CameraControllerBehaviour::deserializeFields(const Serialization::ScriptFieldReader& in)
{
    if (auto v = in.getString("targetCameraName"))
    {
        m_targetCameraName = *v;
    }
}

void CameraControllerBehaviour::bindCameraActions(Components::CInputController& input)
{
    {
        ActionBinding b;
        b.keys    = {KeyCode::Equal, KeyCode::Add};
        b.trigger = ActionTrigger::Held;
        input.bindings["CameraZoomIn"].push_back({b, 0});
    }

    {
        ActionBinding b;
        b.keys    = {KeyCode::Hyphen, KeyCode::Subtract};
        b.trigger = ActionTrigger::Held;
        input.bindings["CameraZoomOut"].push_back({b, 0});
    }

    {
        ActionBinding b;
        b.keys    = {KeyCode::Q};
        b.trigger = ActionTrigger::Held;
        input.bindings["CameraRotateLeft"].push_back({b, 0});
    }

    {
        ActionBinding b;
        b.keys    = {KeyCode::E};
        b.trigger = ActionTrigger::Held;
        input.bindings["CameraRotateRight"].push_back({b, 0});
    }

    {
        ActionBinding b;
        b.keys    = {KeyCode::N};
        b.trigger = ActionTrigger::Pressed;
        input.bindings["CameraNext"].push_back({b, 0});
    }

    {
        ActionBinding b;
        b.keys    = {KeyCode::P};
        b.trigger = ActionTrigger::Pressed;
        input.bindings["CameraPrev"].push_back({b, 0});
    }
}

Entity CameraControllerBehaviour::findCameraEntity(World& world) const
{
    Entity result = Entity::null();

    world.components().view<Components::CCamera>(
        [&](Entity entity, const Components::CCamera& camera)
        {
            if (camera.name == m_targetCameraName)
            {
                result = entity;
            }
        });

    return result;
}

std::vector<std::string> CameraControllerBehaviour::collectEnabledCameraNames(World& world) const
{
    std::vector<std::string> names;

    world.components().view<Components::CCamera>(
        [&](Entity /*entity*/, const Components::CCamera& camera)
        {
            if (camera.enabled)
            {
                names.push_back(camera.name);
            }
        });

    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace Example
