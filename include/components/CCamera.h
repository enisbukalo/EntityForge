#ifndef CCAMERA_H
#define CCAMERA_H

#include <string>

#include "Entity.h"
#include "Vec2.h"

namespace Components
{

/**
 * @brief Data-only camera component.
 *
 * Stores camera configuration and runtime state. Updated by camera systems.
 */
struct CCamera
{
    struct Viewport
    {
        float left   = 0.0f;
        float top    = 0.0f;
        float width  = 1.0f;
        float height = 1.0f;
    };

    struct WorldRect
    {
        Vec2 min = Vec2(0.0f, 0.0f);
        Vec2 max = Vec2(0.0f, 0.0f);
    };

    std::string name    = "Main";
    bool        enabled = true;
    bool        render  = true;

    int renderOrder = 0;

    Entity followTarget  = Entity::null();
    bool   followEnabled = false;
    Vec2   followOffset  = Vec2(0.0f, 0.0f);

    float zoom            = 1.0f;
    float rotationRadians = 0.0f;

    // Camera view size specified in world units (meters). Width is derived from viewport aspect.
    float worldHeight = 10.0f;

    // Used when not following a target.
    Vec2 position = Vec2(0.0f, 0.0f);

    Viewport viewport;

    bool      clampEnabled = false;
    WorldRect clampRect;
};

}  // namespace Components

#endif  // CCAMERA_H
