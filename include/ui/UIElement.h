#pragma once

#include <memory>
#include <vector>

#include <UIDrawList.h>
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

    void setPositionPx(const Vec2& positionPx)
    {
        m_transform.positionPx = positionPx;
    }
    void setPositionPx(float x, float y)
    {
        m_transform.positionPx = Vec2{x, y};
    }

    void setSizePx(const Vec2& sizePx)
    {
        m_transform.sizePx = sizePx;
    }
    void setSizePx(float w, float h)
    {
        m_transform.sizePx = Vec2{w, h};
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

    UIRect rectPx() const
    {
        return UIRect{m_transform.positionPx.x, m_transform.positionPx.y, m_transform.sizePx.x, m_transform.sizePx.y};
    }

    UIElement& addChild(std::unique_ptr<UIElement> child)
    {
        m_children.push_back(std::move(child));
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
    UITransform                             m_transform;
    UIStyle                                 m_style;
    bool                                    m_visible = true;
    std::vector<std::unique_ptr<UIElement>> m_children;
};

}  // namespace UI
