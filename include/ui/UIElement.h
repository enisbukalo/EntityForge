#pragma once

#include <cmath>
#include <memory>
#include <vector>

#include <UIDrawList.h>
#include <UIRect.h>
#include <UIStyle.h>
#include <UITransform.h>

namespace UI
{

class UIElement
{
public:
    UIElement()          = default;
    virtual ~UIElement() = default;

    UIElement(const UIElement&)            = delete;
    UIElement& operator=(const UIElement&) = delete;

    // Legacy manual placement helpers (Phase 1 behavior): treat position as absolute
    // top-left in parent pixel space when anchors are at (0,0).
    void setPositionPx(const Vec2& positionPx)
    {
        const Vec2 currentSize  = sizePx();
        m_transform.offsetMinPx = positionPx;
        m_transform.offsetMaxPx = Vec2{positionPx.x + currentSize.x, positionPx.y + currentSize.y};
        markLayoutDirty();
    }
    void setPositionPx(float x, float y)
    {
        setPositionPx(Vec2{x, y});
    }

    void setSizePx(const Vec2& sizePx)
    {
        m_transform.offsetMaxPx = Vec2{m_transform.offsetMinPx.x + sizePx.x, m_transform.offsetMinPx.y + sizePx.y};
        markLayoutDirty();
    }
    void setSizePx(float w, float h)
    {
        setSizePx(Vec2{w, h});
    }

    Vec2 sizePx() const
    {
        return Vec2{m_transform.offsetMaxPx.x - m_transform.offsetMinPx.x,
                    m_transform.offsetMaxPx.y - m_transform.offsetMinPx.y};
    }

    void setAnchorsNormalized(const Vec2& anchorMin, const Vec2& anchorMax)
    {
        m_transform.anchorMin = anchorMin;
        m_transform.anchorMax = anchorMax;
        markLayoutDirty();
    }
    void setAnchorsNormalized(float minX, float minY, float maxX, float maxY)
    {
        setAnchorsNormalized(Vec2{minX, minY}, Vec2{maxX, maxY});
    }
    const Vec2& anchorMinNormalized() const
    {
        return m_transform.anchorMin;
    }
    const Vec2& anchorMaxNormalized() const
    {
        return m_transform.anchorMax;
    }

    void setPivotNormalized(const Vec2& pivot)
    {
        m_transform.pivot = pivot;
        markLayoutDirty();
    }
    void setPivotNormalized(float x, float y)
    {
        setPivotNormalized(Vec2{x, y});
    }
    const Vec2& pivotNormalized() const
    {
        return m_transform.pivot;
    }

    void setOffsetsPx(const Vec2& offsetMinPx, const Vec2& offsetMaxPx)
    {
        m_transform.offsetMinPx = offsetMinPx;
        m_transform.offsetMaxPx = offsetMaxPx;
        markLayoutDirty();
    }
    void setOffsetsPx(float minX, float minY, float maxX, float maxY)
    {
        setOffsetsPx(Vec2{minX, minY}, Vec2{maxX, maxY});
    }
    const Vec2& offsetMinPx() const
    {
        return m_transform.offsetMinPx;
    }
    const Vec2& offsetMaxPx() const
    {
        return m_transform.offsetMaxPx;
    }

    void setZ(int z)
    {
        m_transform.z = z;
    }

    void setVisible(bool visible)
    {
        m_visible = visible;
    }
    bool isVisible() const
    {
        return m_visible;
    }

    UITransform& transform()
    {
        return m_transform;
    }
    const UITransform& transform() const
    {
        return m_transform;
    }

    UIStyle& style()
    {
        return m_style;
    }
    const UIStyle& style() const
    {
        return m_style;
    }

    // Layout pass: compute this element's rect in absolute pixel space.
    void layout(const UIRect& parentRectPx) const
    {
        const auto nearlyEqual = [](float a, float b) { return std::abs(a - b) <= 1e-6f; };

        const bool parentChanged = !m_hasLastParentRect || (m_lastParentRectPx.x != parentRectPx.x)
                                   || (m_lastParentRectPx.y != parentRectPx.y) || (m_lastParentRectPx.w != parentRectPx.w)
                                   || (m_lastParentRectPx.h != parentRectPx.h);

        if (m_layoutDirty || parentChanged)
        {
            const float anchorMinX = parentRectPx.x + parentRectPx.w * m_transform.anchorMin.x;
            const float anchorMinY = parentRectPx.y + parentRectPx.h * m_transform.anchorMin.y;
            const float anchorMaxX = parentRectPx.x + parentRectPx.w * m_transform.anchorMax.x;
            const float anchorMaxY = parentRectPx.y + parentRectPx.h * m_transform.anchorMax.y;

            float rectMinX = 0.0f;
            float rectMinY = 0.0f;
            float rectMaxX = 0.0f;
            float rectMaxY = 0.0f;

            // Pivot only affects axes that are point-anchored (anchorMin == anchorMax).
            // For stretched axes, offsets represent margins from the anchor rect edges.
            if (nearlyEqual(m_transform.anchorMin.x, m_transform.anchorMax.x))
            {
                const float sizeX = m_transform.offsetMaxPx.x - m_transform.offsetMinPx.x;
                rectMinX          = anchorMinX + m_transform.offsetMinPx.x - (m_transform.pivot.x * sizeX);
                rectMaxX          = rectMinX + sizeX;
            }
            else
            {
                rectMinX = anchorMinX + m_transform.offsetMinPx.x;
                rectMaxX = anchorMaxX + m_transform.offsetMaxPx.x;
            }

            if (nearlyEqual(m_transform.anchorMin.y, m_transform.anchorMax.y))
            {
                const float sizeY = m_transform.offsetMaxPx.y - m_transform.offsetMinPx.y;
                rectMinY          = anchorMinY + m_transform.offsetMinPx.y - (m_transform.pivot.y * sizeY);
                rectMaxY          = rectMinY + sizeY;
            }
            else
            {
                rectMinY = anchorMinY + m_transform.offsetMinPx.y;
                rectMaxY = anchorMaxY + m_transform.offsetMaxPx.y;
            }

            m_computedRectPx.x = rectMinX;
            m_computedRectPx.y = rectMinY;
            m_computedRectPx.w = rectMaxX - rectMinX;
            m_computedRectPx.h = rectMaxY - rectMinY;

            m_lastParentRectPx  = parentRectPx;
            m_hasLastParentRect = true;
            m_layoutDirty       = false;
        }

        for (const auto& child : m_children)
        {
            if (child)
            {
                child->layout(m_computedRectPx);
            }
        }
    }

    UIRect rectPx() const
    {
        return m_computedRectPx;
    }

    UIElement& addChild(std::unique_ptr<UIElement> child)
    {
        m_children.push_back(std::move(child));
        markLayoutDirty();
        return *m_children.back();
    }

    void render(UIDrawList& drawList) const
    {
        if (!m_visible)
        {
            return;
        }

        onRender(drawList);

        for (const auto& child : m_children)
        {
            if (child)
            {
                child->render(drawList);
            }
        }
    }

protected:
    virtual void onRender(UIDrawList& drawList) const
    {
        (void)drawList;
    }

private:
    void markLayoutDirty() const
    {
        m_layoutDirty = true;
    }

    UITransform                             m_transform;
    UIStyle                                 m_style;
    bool                                    m_visible = true;
    std::vector<std::unique_ptr<UIElement>> m_children;

    // Phase 2 layout state (computed at render time)
    mutable bool   m_layoutDirty       = true;
    mutable bool   m_hasLastParentRect = false;
    mutable UIRect m_lastParentRectPx;
    mutable UIRect m_computedRectPx;
};

}  // namespace UI
