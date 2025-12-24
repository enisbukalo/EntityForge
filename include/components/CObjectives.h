#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Components
{

enum class ObjectiveStatus
{
    Inactive = 0,
    InProgress,
    Completed
};

struct ObjectiveInstance
{
    std::string id;

    // UX-facing fields. Later these can be driven by ObjectiveDefinition files.
    std::string title;
    std::string description;

    ObjectiveStatus status{ObjectiveStatus::Inactive};

    // MVP prerequisites (AND semantics). In later phases these can move to authored definitions.
    std::vector<std::string> prerequisites;

    // Generic progress storage: counter bag keyed by string.
    std::unordered_map<std::string, std::int64_t> counters;

    // Idempotency flag for completion reward callbacks.
    bool rewardGranted{false};

    // Optional: script callback name (data-driven later). Not executed by engine in MVP.
    std::string onComplete;
};

/**
 * @brief Stores all objective runtime state for a player/game-state entity.
 */
struct CObjectives
{
    std::vector<ObjectiveInstance> objectives;

    // Objective-manager owned state used by activationConditions
    std::unordered_map<std::string, std::int64_t> managerCounters;
    std::unordered_map<std::string, bool>         managerFlags;

    ObjectiveInstance* tryGetObjective(const std::string& objectiveId)
    {
        for (auto& o : objectives)
        {
            if (o.id == objectiveId)
            {
                return &o;
            }
        }
        return nullptr;
    }

    const ObjectiveInstance* tryGetObjective(const std::string& objectiveId) const
    {
        for (const auto& o : objectives)
        {
            if (o.id == objectiveId)
            {
                return &o;
            }
        }
        return nullptr;
    }

    ObjectiveInstance& getOrAddObjective(const std::string& objectiveId)
    {
        if (auto* existing = tryGetObjective(objectiveId))
        {
            return *existing;
        }

        ObjectiveInstance inst;
        inst.id = objectiveId;
        objectives.push_back(std::move(inst));
        return objectives.back();
    }

    bool canActivateObjective(const std::string& objectiveId) const
    {
        const auto* inst = tryGetObjective(objectiveId);
        if (inst == nullptr)
        {
            return true;
        }

        for (const auto& prereq : inst->prerequisites)
        {
            const auto* p = tryGetObjective(prereq);
            if (p == nullptr || p->status != ObjectiveStatus::Completed)
            {
                return false;
            }
        }

        return true;
    }

    bool activateObjective(const std::string& objectiveId, std::string title = {}, std::string description = {})
    {
        if (!canActivateObjective(objectiveId))
        {
            return false;
        }

        auto& inst = getOrAddObjective(objectiveId);
        if (!title.empty())
        {
            inst.title = std::move(title);
        }
        if (!description.empty())
        {
            inst.description = std::move(description);
        }

        if (inst.status == ObjectiveStatus::Inactive)
        {
            inst.status = ObjectiveStatus::InProgress;
        }
        return true;
    }

    bool setObjectiveStatus(const std::string& objectiveId, ObjectiveStatus status)
    {
        auto& inst  = getOrAddObjective(objectiveId);
        inst.status = status;
        return true;
    }

    bool incrementObjectiveCounter(const std::string& objectiveId, const std::string& key, std::int64_t delta)
    {
        auto* inst = tryGetObjective(objectiveId);
        if (inst == nullptr)
        {
            return false;
        }
        if (inst->status == ObjectiveStatus::Inactive)
        {
            return false;
        }

        inst->counters[key] += delta;
        return true;
    }

    bool isObjectiveCompleted(const std::string& objectiveId) const
    {
        const auto* inst = tryGetObjective(objectiveId);
        return inst != nullptr && inst->status == ObjectiveStatus::Completed;
    }

    bool tryGrantObjectiveReward(const std::string& objectiveId)
    {
        auto* inst = tryGetObjective(objectiveId);
        if (inst == nullptr)
        {
            return false;
        }
        if (inst->status != ObjectiveStatus::Completed)
        {
            return false;
        }
        if (inst->rewardGranted)
        {
            return false;
        }

        inst->rewardGranted = true;
        return true;
    }

    void incrementManagerCounter(const std::string& key, std::int64_t delta)
    {
        managerCounters[key] += delta;
    }

    std::int64_t getManagerCounter(const std::string& key) const
    {
        auto it = managerCounters.find(key);
        if (it == managerCounters.end())
        {
            return 0;
        }
        return it->second;
    }

    void setManagerFlag(const std::string& flag, bool value = true)
    {
        managerFlags[flag] = value;
    }

    bool isManagerFlagSet(const std::string& flag) const
    {
        auto it = managerFlags.find(flag);
        if (it == managerFlags.end())
        {
            return false;
        }
        return it->second;
    }
};

}  // namespace Components
