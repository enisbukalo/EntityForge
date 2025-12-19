#include "SScript.h"

#include <CNativeScript.h>
#include <World.h>
#include "Logger.h"

#include <vector>

void Systems::SScript::update(float deltaTime, World& world)
{
    // Snapshot entities first so scripts can safely spawn entities / add scripts
    // without invalidating the underlying component store iteration.
    std::vector<Entity> scriptedEntities;
    scriptedEntities.reserve(64);

    world.components().view<Components::CNativeScript>([&](Entity entity, Components::CNativeScript& /*script*/)
                                                       { scriptedEntities.push_back(entity); });

    int entityIndex = 0;
    for (Entity entity : scriptedEntities)
    {
        if (!world.isAlive(entity))
        {
            entityIndex++;
            continue;
        }

        auto* script = world.components().tryGet<Components::CNativeScript>(entity);
        if (!script || !script->instance)
        {
            entityIndex++;
            continue;
        }

        if (!script->created)
        {
            LOG_INFO("SScript: onCreate begin E{}:G{} type='{}'", entity.index, entity.generation, script->scriptTypeName);
            script->instance->onCreate(entity, world);
            // Re-fetch script pointer after onCreate because the component store may have
            // reallocated if new scripts were added during onCreate (e.g., spawning entities)
            script = world.components().tryGet<Components::CNativeScript>(entity);
            if (!script || !script->instance)
            {
                entityIndex++;
                continue;
            }

            LOG_INFO("SScript: onCreate end   E{}:G{} type='{}'", entity.index, entity.generation, script->scriptTypeName);

            script->created = true;
        }

        script->instance->onUpdate(deltaTime, entity, world);
        entityIndex++;
    }
}
