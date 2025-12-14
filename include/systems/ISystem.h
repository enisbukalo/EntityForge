#pragma once

#include <string_view>

class World;

namespace Systems
{

enum class UpdateStage
{
    PreFlush,
    PostFlush
};

/**
 * @brief Minimal system interface used by the engine update loop.
 */
class ISystem
{
public:
    virtual ~ISystem() = default;

    /**
     * @brief Per-frame update (variable timestep).
     */
    virtual void update(float deltaTime, World& world) = 0;

    /**
     * @brief Whether this system should be driven by the engine fixed-timestep loop.
     */
    virtual bool usesFixedTimestep() const
    {
        return false;
    }

    /**
     * @brief Fixed-timestep update. Only called when usesFixedTimestep() is true.
     */
    virtual void fixedUpdate(float timeStep, World& world)
    {
        update(timeStep, world);
    }

    /**
     * @brief Controls whether a system runs before or after the world's command buffer flush.
     */
    virtual UpdateStage stage() const
    {
        return UpdateStage::PreFlush;
    }

    /**
     * @brief Returns human-readable name of the system for logging/debugging.
     */
    virtual std::string_view name() const = 0;
};

}  // namespace Systems
