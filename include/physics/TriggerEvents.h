#pragma once

#include <Entity.h>

namespace Physics
{

struct TriggerEnter
{
    Entity triggerEntity{};
    Entity otherEntity{};
};

struct TriggerExit
{
    Entity triggerEntity{};
    Entity otherEntity{};
};

}  // namespace Physics
