#pragma once

#include <Components.h>

namespace Example
{

// Creates the boat entity with its default components (transform, sprite, physics body, collider, script).
// Scene composition code (main) should call this rather than hardcoding boat-specific data.
Entity spawnBoat(World& world);

}  // namespace Example
