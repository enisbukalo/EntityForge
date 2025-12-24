#pragma once

#include <optional>
#include <string>

#include <Color.h>

namespace UI
{

struct UIStyle
{
    Color       backgroundColor = Color::Transparent;
    Color       textColor       = Color::White;
    std::string fontPath;         // Empty means "renderer default".
    unsigned    textSizePx = 16;  // Pixel size.
};

// Phase 5: optional per-element overrides.
// When a field is not set, it will be sourced from the theme (if any) or UIStyle defaults.
struct UIStyleOverrides
{
    std::optional<Color>       backgroundColor;
    std::optional<Color>       textColor;
    std::optional<std::string> fontPath;
    std::optional<unsigned>    textSizePx;
};

}  // namespace UI
