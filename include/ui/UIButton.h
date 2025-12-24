#pragma once

#include <functional>
#include <string>

#include <UIElement.h>

namespace UI
{

class UIButton final : public UIElement
{
public:
    enum class State
    {
        Normal,
        Hovered,
        Pressed,
        Disabled
    };

    UIButton()
    {
        setHitTestVisible(true);
    }

    void setText(std::string text)
    {
        m_text = std::move(text);
    }
    const std::string& text() const
    {
        return m_text;
    }

    void onClick(std::function<void()> cb)
    {
        m_onClick = std::move(cb);
    }

    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
        setInteractable(enabled);
        updateVisualState();
    }
    bool isEnabled() const
    {
        return m_enabled;
    }

    State state() const
    {
        return m_state;
    }

protected:
    void onRender(UIDrawList& drawList) const override
    {
        // Minimal visuals: rect background + label text.
        drawList.addRect(rectPx(), style().backgroundColor, transform().z);

        const UIRect r = rectPx();
        const Vec2   pos{r.x, r.y};
        drawList.addText(pos, m_text, style().textColor, style().fontPath, style().textSizePx, transform().z);
    }

    void onHoverChanged(bool hovered) override
    {
        m_hovered = hovered;
        updateVisualState();
    }

    void onActiveChanged(bool active) override
    {
        m_pressed = active;
        updateVisualState();
    }

    void onFocusChanged(bool focused) override
    {
        m_focused = focused;
        (void)m_focused;
    }

    bool onClick() override
    {
        if (!m_enabled)
        {
            return false;
        }
        if (m_onClick)
        {
            m_onClick();
        }
        return true;
    }

private:
    void updateVisualState()
    {
        if (!m_enabled)
        {
            m_state = State::Disabled;
            return;
        }
        if (m_pressed)
        {
            m_state = State::Pressed;
            return;
        }
        if (m_hovered)
        {
            m_state = State::Hovered;
            return;
        }
        m_state = State::Normal;
    }

    std::string           m_text;
    std::function<void()> m_onClick;
    bool                  m_enabled = true;
    bool                  m_hovered = false;
    bool                  m_pressed = false;
    bool                  m_focused = false;
    State                 m_state   = State::Normal;
};

}  // namespace UI
