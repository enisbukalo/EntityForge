#pragma once

#include <UIElement.h>

namespace UI
{

class UIPanel final : public UIElement
{
protected:
    void onRender(UIDrawList& drawList, const UITheme* theme) const override
    {
        const UIStyle s = resolveStyle(theme);
        drawList.addRect(rectPx(), s.backgroundColor, transform().z);
    }
};

}  // namespace UI
