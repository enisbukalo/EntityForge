#pragma once

#include <UIElement.h>

namespace UI
{

class UIPanel final : public UIElement
{
protected:
    void onRender(UIDrawList& drawList) const override
    {
        drawList.addRect(rectPx(), style().backgroundColor, transform().z);
    }
};

}  // namespace UI
