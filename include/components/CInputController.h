#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include "ActionBinding.h"
#include "InputEvents.h"

namespace Components
{

/**
 * @brief Input controller data component.
 *
 * Stores desired action bindings and the latest observed action states. All
 * behavior (binding to the input system, dispatching callbacks, polling state)
 * lives in Systems::SInput. This component is pure data so systems can decide
 * how to consume or synchronize input for an entity.
 */
struct CInputController
{
    struct Binding
    {
        ActionBinding binding;
        size_t        bindingId{0};
    };

    // Desired bindings for this entity's actions
    std::unordered_map<std::string, std::vector<Binding>> bindings;

    // Latest known action states (filled by systems)
    std::unordered_map<std::string, ActionState> actionStates;

    /**
     * @brief Check if an action is currently active (Pressed or Held).
     * @param action The action name to check.
     * @return true if the action state is Pressed or Held, false otherwise.
     */
    bool isActionActive(const std::string& action) const
    {
        auto it = actionStates.find(action);
        if (it == actionStates.end())
        {
            return false;
        }
        return it->second == ActionState::Pressed || it->second == ActionState::Held;
    }
};

}  // namespace Components
