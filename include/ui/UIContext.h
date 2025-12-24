#pragma once

#include <memory>

#include <UIElement.h>
#include <UIRect.h>

namespace UI
{
class UIContext
{
public:
    UIContext() : m_root(std::make_unique<UIElement>())
    {
        // Root stretches to the full viewport.
        m_root->setAnchorsNormalized(0.0f, 0.0f, 1.0f, 1.0f);
        m_root->setOffsetsPx(0.0f, 0.0f, 0.0f, 0.0f);
    }

    UIElement& root()
    {
        return *m_root;
    }
    const UIElement& root() const
    {
        return *m_root;
    }

    void layout(const UIRect& viewportRectPx) const
    {
        m_root->layout(viewportRectPx);
    }

private:
    std::unique_ptr<UIElement> m_root;
};

}  // namespace UI
