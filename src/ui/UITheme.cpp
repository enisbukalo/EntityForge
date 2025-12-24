#include <UITheme.h>

#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

namespace UI
{

namespace
{

bool parseColor(const nlohmann::json& j, Color& out)
{
    if (!j.is_array())
    {
        return false;
    }
    if (j.size() != 3 && j.size() != 4)
    {
        return false;
    }

    const int r = j.at(0).get<int>();
    const int g = j.at(1).get<int>();
    const int b = j.at(2).get<int>();
    const int a = (j.size() == 4) ? j.at(3).get<int>() : 255;

    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 || a < 0 || a > 255)
    {
        return false;
    }

    out = Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a));
    return true;
}

bool parseStyleObject(const nlohmann::json& j, UIStyle& out, std::string* error)
{
    if (!j.is_object())
    {
        if (error)
        {
            *error = "style must be a JSON object";
        }
        return false;
    }

    // Start from defaults; apply any provided fields.
    UIStyle s{};

    if (j.contains("backgroundColor"))
    {
        Color c;
        if (!parseColor(j.at("backgroundColor"), c))
        {
            if (error)
            {
                *error = "backgroundColor must be [r,g,b] or [r,g,b,a] (0..255)";
            }
            return false;
        }
        s.backgroundColor = c;
    }

    if (j.contains("textColor"))
    {
        Color c;
        if (!parseColor(j.at("textColor"), c))
        {
            if (error)
            {
                *error = "textColor must be [r,g,b] or [r,g,b,a] (0..255)";
            }
            return false;
        }
        s.textColor = c;
    }

    if (j.contains("fontPath"))
    {
        if (!j.at("fontPath").is_string())
        {
            if (error)
            {
                *error = "fontPath must be a string";
            }
            return false;
        }
        s.fontPath = j.at("fontPath").get<std::string>();
    }

    if (j.contains("textSizePx"))
    {
        if (!j.at("textSizePx").is_number_integer())
        {
            if (error)
            {
                *error = "textSizePx must be an integer";
            }
            return false;
        }
        const int size = j.at("textSizePx").get<int>();
        if (size <= 0)
        {
            if (error)
            {
                *error = "textSizePx must be > 0";
            }
            return false;
        }
        s.textSizePx = static_cast<unsigned>(size);
    }

    out = std::move(s);
    return true;
}

}  // namespace

bool UITheme::loadFromFile(const std::string& path, std::vector<std::string>* errors)
{
    std::ifstream in(path);
    if (!in)
    {
        if (errors)
        {
            errors->push_back("UITheme: failed to open file: " + path);
        }
        return false;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();

    nlohmann::json root;
    try
    {
        root = nlohmann::json::parse(buffer.str());
    }
    catch (const std::exception& e)
    {
        if (errors)
        {
            errors->push_back(std::string("UITheme: JSON parse error: ") + e.what());
        }
        return false;
    }

    if (!root.is_object())
    {
        if (errors)
        {
            errors->push_back("UITheme: root must be an object");
        }
        return false;
    }

    const nlohmann::json* stylesObj = &root;
    if (root.contains("styles"))
    {
        if (!root.at("styles").is_object())
        {
            if (errors)
            {
                errors->push_back("UITheme: 'styles' must be an object");
            }
            return false;
        }
        stylesObj = &root.at("styles");
    }

    m_styles.clear();

    bool ok = true;
    for (auto it = stylesObj->begin(); it != stylesObj->end(); ++it)
    {
        const std::string key = it.key();
        UIStyle           style;
        std::string       err;
        if (!parseStyleObject(it.value(), style, &err))
        {
            ok = false;
            if (errors)
            {
                errors->push_back("UITheme: style '" + key + "' invalid: " + err);
            }
            continue;
        }

        m_styles[key] = style;
    }

    return ok;
}

}  // namespace UI
