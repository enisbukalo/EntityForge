#pragma once

#include <string>

#include <UIElement.h>

namespace UI
{

class UILabel final : public UIElement
{
public:
    void setText(std::string text)
    {
        m_text = std::move(text);
    }
    const std::string& text() const
    {
        return m_text;
    }

protected:
    void onRender(UIDrawList& drawList, const UITheme* theme) const override
    {
        const UIStyle s = resolveStyle(theme);
        const UIRect  r = rectPx();
        const Vec2    pos{r.x, r.y};
        drawList.addText(pos, m_text, s.textColor, s.fontPath, s.textSizePx, transform().z);
    }

private:
    std::string m_text;
};

}  // namespace UI
