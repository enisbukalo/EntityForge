#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <UIDrawList.h>
#include <UIRect.h>
#include <UIStyle.h>
#include <UITheme.h>
#include <UITransform.h>

namespace UI
{

namespace detail
{
inline void applyOverrides(UIStyle& base, const UIStyleOverrides& overrides)
{
    if (overrides.backgroundColor.has_value())
    {
        base.backgroundColor = *overrides.backgroundColor;
    }
    if (overrides.textColor.has_value())
    {
        base.textColor = *overrides.textColor;
    }
    if (overrides.fontPath.has_value())
    {
        base.fontPath = *overrides.fontPath;
    }
    if (overrides.textSizePx.has_value())
    {
        base.textSizePx = *overrides.textSizePx;
    }
}
}

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

    // Phase 3: input/hit-testing flags
    void setHitTestVisible(bool hitTestVisible)
    {
        m_hitTestVisible = hitTestVisible;
    }
    bool isHitTestVisible() const
    {
        return m_hitTestVisible;
    }

    void setInteractable(bool interactable)
    {
        m_interactable = interactable;
    }
    bool isInteractable() const
    {
        return m_interactable;
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

    // Phase 5: optional style overrides (merged with theme).
    UIStyleOverrides& styleOverrides()
    {
        return m_styleOverrides;
    }
    const UIStyleOverrides& styleOverrides() const
    {
        return m_styleOverrides;
    }

    void setStyleClass(std::string styleClass)
    {
        m_styleClass = std::move(styleClass);
    }
    const std::string& styleClass() const
    {
        return m_styleClass;
    }

    UIStyle resolveStyle(const UITheme* theme) const;
    UIStyle resolveStyleForClass(const UITheme* theme, const std::string& styleClass) const;

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

        onLayoutChildren(m_computedRectPx);
    }

    // Phase 4: container widgets can assign an absolute rect to a child element.
    // This bypasses the anchor/pivot/offset computation for this element, but still
    // lays out its children relative to the assigned rect.
    void layoutAssigned(const UIRect& absoluteRectPx) const
    {
        m_computedRectPx = absoluteRectPx;

        // Treat the assigned rect as the last parent rect so subsequent conventional
        // layout calls won't accidentally skip child layout due to stale parent tracking.
        m_lastParentRectPx  = absoluteRectPx;
        m_hasLastParentRect = true;
        m_layoutDirty       = false;

        onLayoutChildren(m_computedRectPx);
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
        render(drawList, nullptr);
    }

    void render(UIDrawList& drawList, const UITheme* theme) const
    {
        if (!m_visible)
        {
            return;
        }

        onRender(drawList, theme);

        for (const auto& child : m_children)
        {
            if (child)
            {
                child->render(drawList, theme);
            }
        }
    }

    // Phase 3: hit testing (public API)
    UIElement* hitTest(const Vec2& pointPx)
    {
        return const_cast<UIElement*>(std::as_const(*this).hitTest(pointPx));
    }
    const UIElement* hitTest(const Vec2& pointPx) const
    {
        const UIElement* best      = nullptr;
        int              bestZ     = std::numeric_limits<int>::min();
        uint64_t         bestOrder = 0;
        uint64_t         order     = 0;
        hitTestImpl(pointPx, best, bestZ, bestOrder, order);
        return best;
    }

    // Phase 3: input notifications (public wrappers)
    void notifyHoverChanged(bool hovered)
    {
        onHoverChanged(hovered);
    }
    void notifyActiveChanged(bool active)
    {
        onActiveChanged(active);
    }
    void notifyFocusChanged(bool focused)
    {
        onFocusChanged(focused);
    }
    bool click()
    {
        return onClick();
    }

protected:
    virtual void onRender(UIDrawList& drawList, const UITheme* theme) const
    {
        (void)drawList;
        (void)theme;
    }

    // Phase 4: container widgets override this to compute child rectangles.
    virtual void onLayoutChildren(const UIRect& selfRectPx) const
    {
        for (const auto& child : m_children)
        {
            if (child)
            {
                child->layout(selfRectPx);
            }
        }
    }

    const std::vector<std::unique_ptr<UIElement>>& children() const
    {
        return m_children;
    }

    // Phase 3: input hooks (override points)
    virtual void onHoverChanged(bool /*hovered*/) {}
    virtual void onActiveChanged(bool /*active*/) {}
    virtual void onFocusChanged(bool /*focused*/) {}
    virtual bool onClick()
    {
        return false;
    }

private:
    static bool containsPoint(const UIRect& rect, const Vec2& pointPx)
    {
        const float minX = std::min(rect.x, rect.x + rect.w);
        const float maxX = std::max(rect.x, rect.x + rect.w);
        const float minY = std::min(rect.y, rect.y + rect.h);
        const float maxY = std::max(rect.y, rect.y + rect.h);
        return (pointPx.x >= minX) && (pointPx.x <= maxX) && (pointPx.y >= minY) && (pointPx.y <= maxY);
    }

    void hitTestImpl(const Vec2& pointPx, const UIElement*& best, int& bestZ, uint64_t& bestOrder, uint64_t& order) const
    {
        if (m_visible && m_hitTestVisible && m_interactable)
        {
            const UIRect r = rectPx();
            if (containsPoint(r, pointPx))
            {
                const int z = transform().z;
                if ((z > bestZ) || (z == bestZ && order >= bestOrder))
                {
                    best      = this;
                    bestZ     = z;
                    bestOrder = order;
                }
            }
        }

        ++order;
        for (const auto& child : m_children)
        {
            if (child)
            {
                child->hitTestImpl(pointPx, best, bestZ, bestOrder, order);
            }
        }
    }

    void markLayoutDirty() const
    {
        m_layoutDirty = true;
    }

    UITransform                             m_transform;
    UIStyle                                 m_style;
    std::string                             m_styleClass;
    UIStyleOverrides                        m_styleOverrides;
    bool                                    m_visible        = true;
    bool                                    m_hitTestVisible = false;
    bool                                    m_interactable   = true;
    std::vector<std::unique_ptr<UIElement>> m_children;

    // Phase 2 layout state (computed at render time)
    mutable bool   m_layoutDirty       = true;
    mutable bool   m_hasLastParentRect = false;
    mutable UIRect m_lastParentRectPx;
    mutable UIRect m_computedRectPx;
};

inline UIStyle UIElement::resolveStyle(const UITheme* theme) const
{
    return resolveStyleForClass(theme, m_styleClass);
}

inline UIStyle UIElement::resolveStyleForClass(const UITheme* theme, const std::string& styleClass) const
{
    UIStyle resolved = m_style;

    if (theme != nullptr && !styleClass.empty())
    {
        const auto themed = theme->tryGetStyle(styleClass);
        if (themed.has_value())
        {
            resolved = *themed;
        }
    }

    detail::applyOverrides(resolved, m_styleOverrides);
    return resolved;
}

}  // namespace UI
