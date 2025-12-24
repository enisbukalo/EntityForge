#include <UIRenderer.h>

#include <algorithm>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>

#include <Logger.h>
#include <SRenderer.h>

#include <UIContext.h>
#include <UIDrawList.h>

namespace UI
{

namespace
{

sf::Color toSFMLColor(const Color& c)
{
    return sf::Color(c.r, c.g, c.b, c.a);
}

class FontCache
{
public:
    const sf::Font* getOrLoad(const std::string& path)
    {
        if (path.empty())
        {
            return nullptr;
        }

        auto it = m_fonts.find(path);
        if (it != m_fonts.end())
        {
            return &it->second;
        }

        sf::Font font;
        if (!font.openFromFile(path))
        {
            LOG_WARN("UIRenderer: failed to load font '{}'", path);
            return nullptr;
        }

        auto [insertIt, inserted] = m_fonts.emplace(path, std::move(font));
        (void)inserted;
        return &insertIt->second;
    }

private:
    std::unordered_map<std::string, sf::Font> m_fonts;
};

FontCache& fontCache()
{
    static FontCache cache;
    return cache;
}

}  // namespace

void UIRenderer::setDefaultFontPath(std::string fontPath)
{
    m_defaultFontPath = std::move(fontPath);
}

void UIRenderer::render(const UIContext& context, Systems::SRenderer& renderer)
{
    sf::RenderWindow* window = renderer.getWindow();
    if (!window)
    {
        return;
    }

    // UI is screen-space (pixels), so ensure default view.
    window->setView(window->getDefaultView());

    const auto   winSize = window->getSize();
    const UIRect viewportRectPx{0.0f, 0.0f, static_cast<float>(winSize.x), static_cast<float>(winSize.y)};
    context.setViewportRectPx(viewportRectPx);
    context.layout(viewportRectPx);

    UIDrawList drawList;
    context.root().render(drawList, context.theme());

    std::vector<UIDrawCommand> commands = drawList.commands();
    std::stable_sort(commands.begin(),
                     commands.end(),
                     [](const UIDrawCommand& a, const UIDrawCommand& b) { return a.z < b.z; });

    for (const auto& cmd : commands)
    {
        if (std::holds_alternative<UIDrawRect>(cmd.payload))
        {
            const auto& r = std::get<UIDrawRect>(cmd.payload);

            sf::RectangleShape rect;
            rect.setPosition(sf::Vector2f{r.rectPx.x, r.rectPx.y});
            rect.setSize(sf::Vector2f{r.rectPx.w, r.rectPx.h});
            rect.setFillColor(toSFMLColor(r.color));
            window->draw(rect);
            continue;
        }

        if (std::holds_alternative<UIDrawText>(cmd.payload))
        {
            const auto& t = std::get<UIDrawText>(cmd.payload);

            const std::string& fontPath = t.fontPath.empty() ? m_defaultFontPath : t.fontPath;
            const sf::Font*    font     = fontCache().getOrLoad(fontPath);
            if (!font)
            {
                continue;
            }

            sf::Text text(*font, sf::String(t.text), t.sizePx);
            text.setFillColor(toSFMLColor(t.color));
            text.setPosition(sf::Vector2f{t.positionPx.x, t.positionPx.y});
            window->draw(text);
            continue;
        }
    }
}

}  // namespace UI
