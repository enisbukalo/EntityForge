#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Objectives
{

enum class ActivationConditionType
{
    FlagSet = 0,
    CounterAtLeast
};

struct ActivationCondition
{
    ActivationConditionType type{ActivationConditionType::FlagSet};

    // For FlagSet
    std::string flag;

    // For CounterAtLeast
    std::string  key;
    std::int64_t value{0};
};

enum class ProgressionMode
{
    Targeted = 0,
    Signals,
    Triggers
};

enum class TriggerEventType
{
    Enter = 0,
    Exit
};

enum class TriggerAction
{
    Complete = 0,
    IncrementCounter
};

struct TriggerRule
{
    TriggerEventType type{TriggerEventType::Enter};

    // MVP: reference trigger entity via CName
    std::string triggerName;

    TriggerAction action{TriggerAction::Complete};

    // Used when action == IncrementCounter
    std::string  key;
    std::int64_t delta{1};
};

struct ObjectiveProgression
{
    ProgressionMode mode{ProgressionMode::Targeted};

    // When mode == Signals
    std::vector<std::string> signals;

    // Optional: number of matching signals required to complete the objective.
    // Defaults to 1 to preserve legacy behavior.
    std::int64_t signalCount{1};

    // When mode == Triggers
    std::vector<TriggerRule> triggers;
};

struct ObjectiveDefinition
{
    std::string              id;
    std::string              title;
    std::string              description;
    std::vector<std::string> prerequisites;

    std::vector<ActivationCondition> activationConditions;

    ObjectiveProgression progression;

    // MVP: script-facing callback name invoked on completion.
    std::string onComplete;

    // Optional: helps error reporting/debugging.
    std::filesystem::path sourceFile;
};

}  // namespace Objectives
