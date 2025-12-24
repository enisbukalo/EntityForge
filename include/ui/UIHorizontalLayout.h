#pragma once

#include <algorithm>

#include <UIElement.h>

namespace UI
{

class UIHorizontalLayout final : public UIElement
{
public:
    enum class CrossAxisAlignment
    {
        Start,
        Center,
        End,
        Stretch
    };

    struct Padding
    {
        float left   = 0.0f;
        float top    = 0.0f;
        float right  = 0.0f;
        float bottom = 0.0f;
    };

    void setPaddingPx(const Padding& padding)
    {
        m_padding = padding;
    }
    void setPaddingPx(float left, float top, float right, float bottom)
    {
        m_padding = Padding{left, top, right, bottom};
    }
    Padding paddingPx() const
    {
        return m_padding;
    }

    void setSpacingPx(float spacingPx)
    {
        m_spacingPx = spacingPx;
    }
    float spacingPx() const
    {
        return m_spacingPx;
    }

    void setCrossAxisAlignment(CrossAxisAlignment alignment)
    {
        m_crossAlign = alignment;
    }
    CrossAxisAlignment crossAxisAlignment() const
    {
        return m_crossAlign;
    }

protected:
    void onLayoutChildren(const UIRect& selfRectPx) const override
    {
        const float innerX = selfRectPx.x + m_padding.left;
        const float innerY = selfRectPx.y + m_padding.top;
        const float innerW = std::max(0.0f, selfRectPx.w - (m_padding.left + m_padding.right));
        const float innerH = std::max(0.0f, selfRectPx.h - (m_padding.top + m_padding.bottom));

        (void)innerW;  // Not used yet (no scroll/clipping in Phase 4)

        float cursorX = innerX;
        for (const auto& child : children())
        {
            if (!child)
            {
                continue;
            }

            const Vec2  preferred = child->sizePx();
            const float childW    = preferred.x;
            const float childH    = (m_crossAlign == CrossAxisAlignment::Stretch) ? innerH : preferred.y;

            float childY = innerY;
            if (m_crossAlign == CrossAxisAlignment::Center)
            {
                childY = innerY + (innerH - childH) * 0.5f;
            }
            else if (m_crossAlign == CrossAxisAlignment::End)
            {
                childY = innerY + (innerH - childH);
            }

            child->layoutAssigned(UIRect{cursorX, childY, childW, childH});
            cursorX += childW + m_spacingPx;
        }
    }

private:
    Padding            m_padding{};
    float              m_spacingPx  = 0.0f;
    CrossAxisAlignment m_crossAlign = CrossAxisAlignment::Stretch;
};

}  // namespace UI
