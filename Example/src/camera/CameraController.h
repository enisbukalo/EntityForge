#pragma once

#include <string_view>

#include <Entity.h>

class World;

namespace Example
{

// Creates a camera controller entity that handles zoom, rotation, and camera switching.
// The controller will manipulate the camera with the given name.
Entity spawnCameraController(World& world, std::string_view targetCameraName);

}  // namespace Example
