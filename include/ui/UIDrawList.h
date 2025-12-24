#pragma once

#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <Color.h>
#include <Vec2.h>

#include <UIRect.h>

namespace UI
{

struct UIDrawRect
{
    UIRect rectPx;
    Color  color;
};

struct UIDrawText
{
    Vec2        positionPx;
    std::string text;
    Color       color;
    std::string fontPath;
    unsigned    sizePx = 16;
};

struct UIDrawCommand
{
    using Payload = std::variant<UIDrawRect, UIDrawText>;

    int     z = 0;
    Payload payload;
};

class UIDrawList
{
public:
    void clear()
    {
        m_commands.clear();
    }

    void addRect(const UIRect& rectPx, const Color& color, int z)
    {
        m_commands.push_back(UIDrawCommand{z, UIDrawRect{rectPx, color}});
    }

    void addText(const Vec2& positionPx, std::string text, const Color& color, std::string fontPath, unsigned sizePx, int z)
    {
        UIDrawText t;
        t.positionPx = positionPx;
        t.text       = std::move(text);
        t.color      = color;
        t.fontPath   = std::move(fontPath);
        t.sizePx     = sizePx;
        m_commands.push_back(UIDrawCommand{z, std::move(t)});
    }

    const std::vector<UIDrawCommand>& commands() const
    {
        return m_commands;
    }

private:
    std::vector<UIDrawCommand> m_commands;
};

}  // namespace UI
