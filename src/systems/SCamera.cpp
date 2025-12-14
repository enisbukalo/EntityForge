#include "SCamera.h"

#include <algorithm>

#include <CCamera.h>
#include <CTransform.h>
#include <World.h>

#include <CameraProjection.h>
#include <SRenderer.h>
#include <SystemLocator.h>

#include "Logger.h"

namespace
{
Vec2 clampVec2(const Vec2& value, const Vec2& minValue, const Vec2& maxValue)
{
    return Vec2(std::max(minValue.x, std::min(value.x, maxValue.x)), std::max(minValue.y, std::min(value.y, maxValue.y)));
}
}  // namespace

namespace Systems
{

void SCamera::update(float /*deltaTime*/, World& world)
{
    world.components().viewSorted<Components::CCamera>(
        [&](Entity cameraEntity, Components::CCamera& camera)
        {
            if (!camera.enabled)
            {
                m_warnedInvalidFollow.erase(cameraEntity);
                return;
            }

            bool shouldWarnInvalidFollow = false;
            bool followApplied           = false;

            if (camera.followEnabled)
            {
                if (!camera.followTarget.isValid())
                {
                    shouldWarnInvalidFollow = true;
                }
                else if (!world.isAlive(camera.followTarget))
                {
                    shouldWarnInvalidFollow = true;
                }
                else
                {
                    auto* targetTransform = world.components().tryGet<Components::CTransform>(camera.followTarget);
                    if (!targetTransform)
                    {
                        shouldWarnInvalidFollow = true;
                    }
                    else
                    {
                        camera.position = targetTransform->position + camera.followOffset;
                        followApplied   = true;
                    }
                }
            }

            if (camera.clampEnabled)
            {
                camera.position = clampVec2(camera.position, camera.clampRect.min, camera.clampRect.max);
            }

            if (camera.followEnabled)
            {
                if (followApplied)
                {
                    m_warnedInvalidFollow.erase(cameraEntity);
                }
                else if (shouldWarnInvalidFollow && m_warnedInvalidFollow.insert(cameraEntity).second)
                {
                    LOG_WARN("Camera '{}' follow target invalid (cameraEntity={}, targetEntity={})",
                             camera.name,
                             cameraEntity.index,
                             camera.followTarget.index);
                }
            }
            else
            {
                m_warnedInvalidFollow.erase(cameraEntity);
            }
        });
}

const Components::CCamera* SCamera::getCameraByName(const World& world, std::string_view name) const
{
    const Components::CCamera* found = nullptr;

    world.components().view<Components::CCamera>(
        [&](Entity /*entity*/, const Components::CCamera& camera)
        {
            if (!found && camera.name == name)
            {
                found = &camera;
            }
        });

    return found;
}

const Components::CCamera* SCamera::getActiveCamera(const World& world) const
{
    // Prefer the configured active camera name, but only if it exists and is enabled.
    if (const auto* named = getCameraByName(world, m_activeCameraName))
    {
        if (named->enabled)
        {
            return named;
        }
    }

    // Fall back to an enabled "Main" camera.
    if (m_activeCameraName != "Main")
    {
        if (const auto* mainCam = getCameraByName(world, "Main"))
        {
            if (mainCam->enabled)
            {
                return mainCam;
            }
        }
    }

    // Deterministic fallback: first enabled camera by entity order.
    const Components::CCamera* firstEnabled = nullptr;
    world.components().viewSorted<Components::CCamera>(
        [&](Entity /*entity*/, const Components::CCamera& camera)
        {
            if (!firstEnabled && camera.enabled)
            {
                firstEnabled = &camera;
            }
        });

    return firstEnabled;
}

bool SCamera::setActiveCamera(std::string name)
{
    if (name == m_activeCameraName)
    {
        return false;
    }

    m_activeCameraName = std::move(name);
    LOG_INFO("Active camera set to '{}'", m_activeCameraName);
    return true;
}

Vec2 SCamera::screenToWorld(const World& world, std::string_view cameraName, const Vec2i& windowPx) const
{
    const Components::CCamera* camera = nullptr;
    if (!cameraName.empty())
    {
        camera = getCameraByName(world, cameraName);
    }
    if (!camera)
    {
        camera = getActiveCamera(world);
    }
    if (!camera)
    {
        return Vec2(0.0f, 0.0f);
    }

    sf::Vector2u windowSizePx(0, 0);
    if (auto* renderer = Systems::SystemLocator::tryRenderer())
    {
        if (auto* window = renderer->getWindow())
        {
            windowSizePx = window->getSize();
        }
    }

    return Internal::screenToWorld(*camera, windowSizePx, windowPx);
}

Vec2i SCamera::worldToScreen(const World& world, std::string_view cameraName, const Vec2& worldPos) const
{
    const Components::CCamera* camera = nullptr;
    if (!cameraName.empty())
    {
        camera = getCameraByName(world, cameraName);
    }
    if (!camera)
    {
        camera = getActiveCamera(world);
    }
    if (!camera)
    {
        return Vec2i{0, 0};
    }

    sf::Vector2u windowSizePx(0, 0);
    if (auto* renderer = Systems::SystemLocator::tryRenderer())
    {
        if (auto* window = renderer->getWindow())
        {
            windowSizePx = window->getSize();
        }
    }

    return Internal::worldToScreen(*camera, windowSizePx, worldPos);
}

}  // namespace Systems
