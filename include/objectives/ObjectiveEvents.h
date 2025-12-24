#pragma once

#include <cstdint>
#include <string>

namespace Objectives
{

// Objective-manager state updates
// These are not tied to a specific objective instance; they feed activationConditions.
struct ObjectiveManagerCounterIncrement
{
    std::string  key;
    std::int64_t delta{1};
};

struct ObjectiveManagerFlagSet
{
    std::string flag;
    bool        value{true};
};

// Explicit activation request
// Game code/scripts can emit this; SObjectives enforces prerequisites + activationConditions.
struct ObjectiveActivate
{
    std::string objectiveId;
};

// Targeted objective updates
struct ObjectiveCounterIncrement
{
    std::string  objectiveId;
    std::string  key;
    std::int64_t delta{1};
};

struct ObjectiveComplete
{
    std::string objectiveId;
};

// Generic named signal (payload omitted for MVP)
struct ObjectiveSignal
{
    std::string signalId;
};

}  // namespace Objectives
