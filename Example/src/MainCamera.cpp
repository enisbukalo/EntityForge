#include "MainCamera.h"

#include <Components.h>
#include <World.h>

namespace Example
{

Entity spawnMainCamera(World& world, Entity followTarget, float worldHeightMeters)
{
    Entity cameraEntity = world.createEntity();
    auto*  camera       = world.components().add<Components::CCamera>(cameraEntity);

    camera->name        = "Main";
    camera->enabled     = true;
    camera->render      = true;
    camera->renderOrder = 0;

    camera->viewport.left   = 0.0f;
    camera->viewport.top    = 0.0f;
    camera->viewport.width  = 1.0f;
    camera->viewport.height = 1.0f;

    camera->followEnabled = true;
    camera->followTarget  = followTarget;
    camera->followOffset  = Vec2(0.0f, 0.0f);

    camera->worldHeight     = worldHeightMeters;
    camera->zoom            = 1.0f;
    camera->rotationRadians = 0.0f;

    return cameraEntity;
}

}  // namespace Example
