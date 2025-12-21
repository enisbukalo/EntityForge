#include "CameraController.h"

#include "CameraControllerBehaviour.h"

#include <Components.h>
#include <World.h>

namespace Example
{

Entity spawnCameraController(World& world, std::string_view targetCameraName)
{
    Entity controller = world.createEntity();

    Components::CNativeScript* script = world.components().add<Components::CNativeScript>(controller);
    script->bind<CameraControllerBehaviour>(targetCameraName);

    return controller;
}

}  // namespace Example
