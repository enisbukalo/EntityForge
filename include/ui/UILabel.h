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
    void onRender(UIDrawList& drawList) const override
    {
        const Vec2 pos = transform().positionPx;
        drawList.addText(pos, m_text, style().textColor, style().fontPath, style().textSizePx, transform().z);
    }

private:
    std::string m_text;
};

}  // namespace UI
