#pragma once

#include <Entity.h>

class World;

namespace Example
{

// Creates a single full-viewport camera that follows an entity.
// The camera uses world units (meters). worldHeightMeters should typically match the
// visible world height you want (e.g. windowHeightPx / pixelsPerMeter).
Entity spawnMainCamera(World& world, Entity followTarget, float worldHeightMeters);

}  // namespace Example
