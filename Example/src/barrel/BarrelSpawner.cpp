#include "BarrelSpawner.h"

#include "BarrelSpawnerBehaviour.h"

#include <Components.h>
#include <World.h>

namespace Example
{

Entity spawnBarrelSpawner(World& world)
{
    Entity                     spawner = world.createEntity();
    Components::CNativeScript* script  = world.components().add<Components::CNativeScript>(spawner);
    script->bind<BarrelSpawnerBehaviour>();
    return spawner;
}

Entity spawnBarrelSpawner(World& world, float minX, float maxX, float minY, float maxY, size_t count)
{
    Entity                     spawner = world.createEntity();
    Components::CNativeScript* script  = world.components().add<Components::CNativeScript>(spawner);
    script->bind<BarrelSpawnerBehaviour>(minX, maxX, minY, maxY, count);
    return spawner;
}

}  // namespace Example
