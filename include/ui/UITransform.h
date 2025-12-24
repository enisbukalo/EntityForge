#pragma once

#include <Vec2.h>

namespace UI
{

struct UITransform
{
    Vec2 positionPx{0.0f, 0.0f};
    Vec2 sizePx{0.0f, 0.0f};
    int  z = 0;
};

}  // namespace UI
