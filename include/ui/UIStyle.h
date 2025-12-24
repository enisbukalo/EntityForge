#pragma once

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

}  // namespace UI
