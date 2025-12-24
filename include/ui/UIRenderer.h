#pragma once

#include <string>

namespace Systems
{
class SRenderer;
}

namespace UI
{
class UIContext;

class UIRenderer
{
public:
    UIRenderer()  = default;
    ~UIRenderer() = default;

    UIRenderer(const UIRenderer&)            = delete;
    UIRenderer& operator=(const UIRenderer&) = delete;

    void setDefaultFontPath(std::string fontPath);
    void render(const UIContext& context, Systems::SRenderer& renderer);

private:
    std::string m_defaultFontPath;
};

}  // namespace UI
