#pragma once

#include <Vec2.h>

namespace UI
{

struct UITransform
{
    // Phase 2 positioning model:
    // - Anchors are normalized (0..1) in parent space.
    // - Offsets are pixel offsets from the anchor rect.
    // The final pixel rect is computed during the layout pass.
    Vec2 anchorMin{0.0f, 0.0f};
    Vec2 anchorMax{0.0f, 0.0f};
    Vec2 pivot{0.0f, 0.0f};

    Vec2 offsetMinPx{0.0f, 0.0f};
    Vec2 offsetMaxPx{0.0f, 0.0f};
    int  z = 0;
};

}  // namespace UI
