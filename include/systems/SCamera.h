#pragma once

#include <string>
#include <string_view>
#include <unordered_set>

#include "Entity.h"
#include "System.h"

namespace Components
{
struct CCamera;
}

class World;

namespace Systems
{

/**
 * @brief Camera system responsible for updating camera component state (follow/clamp) and active camera selection.
 *
 * Runs in PreFlush.
 */
class SCamera : public System
{
public:
    SCamera()           = default;
    ~SCamera() override = default;

    void update(float deltaTime, World& world) override;

    std::string_view name() const override
    {
        return "SCamera";
    }

    const Components::CCamera* getActiveCamera(const World& world) const;
    const Components::CCamera* getCameraByName(const World& world, std::string_view name) const;

    bool setActiveCamera(std::string name);

    const std::string& getActiveCameraName() const
    {
        return m_activeCameraName;
    }

private:
    std::string m_activeCameraName = "Main";

    // Track invalid follow targets we've already warned about to avoid per-frame spam.
    std::unordered_set<Entity> m_warnedInvalidFollow;
};

}  // namespace Systems
