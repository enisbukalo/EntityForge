#pragma once

#include "System.h"

namespace Systems
{

/**
 * @brief Runs per-entity native scripts (behaviours) stored in Components::CNativeScript.
 */
class SScript : public System
{
public:
    void update(float deltaTime, World& world) override;

    std::string_view name() const override
    {
        return "SScript";
    }
};

}  // namespace Systems
