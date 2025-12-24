#pragma once

#include <memory>

#include <UIElement.h>

namespace UI
{
class UIContext
{
public:
    UIContext() : m_root(std::make_unique<UIElement>()) {}

    UIElement& root()
    {
        return *m_root;
    }
    const UIElement& root() const
    {
        return *m_root;
    }

private:
    std::unique_ptr<UIElement> m_root;
};

}  // namespace UI
