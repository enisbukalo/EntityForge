#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <UIStyle.h>

namespace UI
{

class UITheme
{
public:
    void clear()
    {
        m_styles.clear();
    }

    void setStyle(std::string styleClass, const UIStyle& style)
    {
        m_styles[std::move(styleClass)] = style;
    }

    std::optional<UIStyle> tryGetStyle(const std::string& styleClass) const
    {
        const auto it = m_styles.find(styleClass);
        if (it == m_styles.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    // Loads a theme JSON file.
    // Supported schema:
    // - { "styles": { "class": { ...style... }, ... } }
    // - or { "class": { ...style... }, ... }
    bool loadFromFile(const std::string& path, std::vector<std::string>* errors = nullptr);

private:
    std::unordered_map<std::string, UIStyle> m_styles;
};

}  // namespace UI
