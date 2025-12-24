#pragma once

#include <Vec2.h>

namespace UI
{

struct UIRect
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    constexpr float left() const
    {
        return x;
    }
    constexpr float top() const
    {
        return y;
    }
    constexpr float right() const
    {
        return x + w;
    }
    constexpr float bottom() const
    {
        return y + h;
    }

    bool containsPx(const Vec2& pointPx) const
    {
        return pointPx.x >= left() && pointPx.x <= right() && pointPx.y >= top() && pointPx.y <= bottom();
    }
};

}  // namespace UI
