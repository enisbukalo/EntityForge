#include "CameraController.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

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

Entity spawnCameraController(World& world, std::string_view targetCameraName)
{
    Entity controller = world.createEntity();

    auto* script = world.components().add<Components::CNativeScript>(controller);
    script->bind<CameraController>(targetCameraName);

    return controller;
}

CameraController::CameraController(std::string_view targetCameraName) : m_targetCameraName(targetCameraName) {}

void CameraController::onCreate(Entity self, World& world)
{
    // Add input controller component for action bindings
    auto* input = world.components().tryGet<Components::CInputController>(self);
    if (!input)
    {
        input = world.components().add<Components::CInputController>(self);
    }
    bindCameraActions(*input);

    // Subscribe to raw input events for scroll wheel
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

void CameraController::onUpdate(float deltaTime, Entity self, World& world)
{
    // Get our input controller
    auto* input = world.components().tryGet<Components::CInputController>(self);
    if (!input)
    {
        return;
    }

    // Find the camera entity we're controlling
    Entity cameraEntity = findCameraEntity(world);
    if (!cameraEntity.isValid())
    {
        return;
    }

    auto* camera = world.components().tryGet<Components::CCamera>(cameraEntity);
    if (!camera)
    {
        return;
    }

    // --- Zoom controls ---
    float zoomChange = 0.0f;

    if (input->isActionActive("CameraZoomIn"))
    {
        zoomChange -= m_zoomSpeed * deltaTime;
    }
    if (input->isActionActive("CameraZoomOut"))
    {
        zoomChange += m_zoomSpeed * deltaTime;
    }

    // Apply scroll wheel zoom (scroll up = zoom in = decrease zoom value)
    zoomChange += m_scrollDelta * m_scrollZoomSpeed;
    m_scrollDelta = 0.0f;

    camera->zoom += zoomChange;
    camera->zoom = std::clamp(camera->zoom, kMinZoom, kMaxZoom);

    // --- Rotation controls ---
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

    // Wrap rotation to [0, 2π)
    constexpr float kTwoPi = 2.0f * 3.14159265358979323846f;
    while (camera->rotationRadians < 0.0f)
    {
        camera->rotationRadians += kTwoPi;
    }
    while (camera->rotationRadians >= kTwoPi)
    {
        camera->rotationRadians -= kTwoPi;
    }

    // --- Camera switching ---
    const bool nextPressed = input->actionStates.count("CameraNext") && input->actionStates.at("CameraNext") == ActionState::Pressed;
    const bool prevPressed = input->actionStates.count("CameraPrev") && input->actionStates.at("CameraPrev") == ActionState::Pressed;

    if (nextPressed || prevPressed)
    {
        auto cameraNames = collectEnabledCameraNames(world);
        if (cameraNames.size() > 1)
        {
            // Find current camera's index
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

CameraController::~CameraController()
{
    // Unsubscribe from input events
    if (m_subscriberId != 0)
    {
        Systems::SystemLocator::input().unsubscribe(m_subscriberId);
        m_subscriberId = 0;
    }
}

void CameraController::bindCameraActions(Components::CInputController& input)
{
    // Zoom In: = (Equal) and Numpad +
    {
        ActionBinding b;
        b.keys    = {KeyCode::Equal, KeyCode::Add};
        b.trigger = ActionTrigger::Held;
        input.bindings["CameraZoomIn"].push_back({b, 0});
    }

    // Zoom Out: - (Minus) and Numpad -
    {
        ActionBinding b;
        b.keys    = {KeyCode::Hyphen, KeyCode::Subtract};
        b.trigger = ActionTrigger::Held;
        input.bindings["CameraZoomOut"].push_back({b, 0});
    }

    // Rotate Left: Q
    {
        ActionBinding b;
        b.keys    = {KeyCode::Q};
        b.trigger = ActionTrigger::Held;
        input.bindings["CameraRotateLeft"].push_back({b, 0});
    }

    // Rotate Right: E
    {
        ActionBinding b;
        b.keys    = {KeyCode::E};
        b.trigger = ActionTrigger::Held;
        input.bindings["CameraRotateRight"].push_back({b, 0});
    }

    // Next Camera: N
    {
        ActionBinding b;
        b.keys    = {KeyCode::N};
        b.trigger = ActionTrigger::Pressed;
        input.bindings["CameraNext"].push_back({b, 0});
    }

    // Previous Camera: P
    {
        ActionBinding b;
        b.keys    = {KeyCode::P};
        b.trigger = ActionTrigger::Pressed;
        input.bindings["CameraPrev"].push_back({b, 0});
    }
}

Entity CameraController::findCameraEntity(World& world) const
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

std::vector<std::string> CameraController::collectEnabledCameraNames(World& world) const
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

    // Sort for deterministic ordering
    std::sort(names.begin(), names.end());
    return names;
}

}  // namespace Example
