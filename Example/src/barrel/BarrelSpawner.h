#pragma once

#include <cstddef>

#include <Entity.h>

class World;

namespace Example
{

Entity spawnBarrelSpawner(World& world);
Entity spawnBarrelSpawner(World& world, float minX, float maxX, float minY, float maxY, size_t count);

}  // namespace Example
