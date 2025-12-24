#pragma once

#include <memory>

#include <InputEvents.h>

#include <UIElement.h>
#include <UIRect.h>

namespace UI
{
class UIContext
{
public:
    UIContext() : m_root(std::make_unique<UIElement>())
    {
        // Root stretches to the full viewport.
        m_root->setAnchorsNormalized(0.0f, 0.0f, 1.0f, 1.0f);
        m_root->setOffsetsPx(0.0f, 0.0f, 0.0f, 0.0f);
    }

    UIElement& root()
    {
        return *m_root;
    }
    const UIElement& root() const
    {
        return *m_root;
    }

    void layout(const UIRect& viewportRectPx) const
    {
        m_root->layout(viewportRectPx);
    }

    // Phase 3: store viewport so input can run a layout pass before hit-testing.
    void setViewportRectPx(const UIRect& viewportRectPx) const
    {
        m_viewportRectPx    = viewportRectPx;
        m_hasViewportRectPx = true;
    }

    struct InputState
    {
        UIElement* hovered = nullptr;
        UIElement* active  = nullptr;
        UIElement* focused = nullptr;
        Vec2i      mousePx{0, 0};
    };

    const InputState& inputState() const
    {
        return m_input;
    }

    // Returns true if UI consumed the event.
    bool handleInputEvent(const InputEvent& inputEvent)
    {
        // Keep layout current for hit-testing.
        if (m_hasViewportRectPx)
        {
            layout(m_viewportRectPx);
        }

        switch (inputEvent.type)
        {
            case InputEventType::MouseMoved:
                return handleMouseMoved(inputEvent.mouseMove.position);
            case InputEventType::MouseButtonPressed:
                return handleMousePressed(inputEvent.mouse.button, inputEvent.mouse.position);
            case InputEventType::MouseButtonReleased:
                return handleMouseReleased(inputEvent.mouse.button, inputEvent.mouse.position);
            default:
                return false;
        }
    }

private:
    bool handleMouseMoved(const Vec2i& posPx)
    {
        m_input.mousePx = posPx;
        const Vec2 point{static_cast<float>(posPx.x), static_cast<float>(posPx.y)};

        UIElement* newHovered = m_root->hitTest(point);
        if (newHovered != m_input.hovered)
        {
            if (m_input.hovered)
            {
                m_input.hovered->notifyHoverChanged(false);
            }
            m_input.hovered = newHovered;
            if (m_input.hovered)
            {
                m_input.hovered->notifyHoverChanged(true);
            }
        }

        // Hovering itself isn't considered consuming input.
        return false;
    }

    bool handleMousePressed(MouseButton button, const Vec2i& posPx)
    {
        (void)handleMouseMoved(posPx);
        if (button != MouseButton::Left)
        {
            return false;
        }

        // Only capture if we're currently hovered (keeps behavior deterministic).
        if (!m_input.hovered)
        {
            return false;
        }

        if (m_input.active && m_input.active != m_input.hovered)
        {
            m_input.active->notifyActiveChanged(false);
        }

        m_input.active = m_input.hovered;
        m_input.active->notifyActiveChanged(true);

        if (m_input.focused && m_input.focused != m_input.active)
        {
            m_input.focused->notifyFocusChanged(false);
        }
        m_input.focused = m_input.active;
        m_input.focused->notifyFocusChanged(true);

        return true;
    }

    bool handleMouseReleased(MouseButton button, const Vec2i& posPx)
    {
        (void)handleMouseMoved(posPx);
        if (button != MouseButton::Left)
        {
            return false;
        }

        if (!m_input.active)
        {
            return false;
        }

        // Click only if still hovered.
        if (m_input.active == m_input.hovered)
        {
            (void)m_input.active->click();
        }

        m_input.active->notifyActiveChanged(false);
        m_input.active = nullptr;
        return true;
    }

    std::unique_ptr<UIElement> m_root;

    mutable bool   m_hasViewportRectPx = false;
    mutable UIRect m_viewportRectPx;

    InputState m_input;
};

}  // namespace UI
