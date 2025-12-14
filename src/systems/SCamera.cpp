#include "SCamera.h"

#include <algorithm>

#include <CCamera.h>
#include <CTransform.h>
#include <World.h>

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

}  // namespace Systems
