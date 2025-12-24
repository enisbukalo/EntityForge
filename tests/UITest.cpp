#include <gtest/gtest.h>

#include <UIContext.h>
#include <UILabel.h>
#include <UIPanel.h>

TEST(UIPhase1, PanelEmitsRectCommand)
{
    UI::UIPanel panel;
    panel.setPositionPx(10.0f, 20.0f);
    panel.setSizePx(100.0f, 50.0f);
    panel.setZ(3);
    panel.style().backgroundColor = Color(1, 2, 3, 4);

    UI::UIDrawList list;
    panel.render(list);

    ASSERT_EQ(list.commands().size(), 1u);
    const auto& cmd = list.commands()[0];
    EXPECT_EQ(cmd.z, 3);

    ASSERT_TRUE(std::holds_alternative<UI::UIDrawRect>(cmd.payload));
    const auto& r = std::get<UI::UIDrawRect>(cmd.payload);
    EXPECT_FLOAT_EQ(r.rectPx.x, 10.0f);
    EXPECT_FLOAT_EQ(r.rectPx.y, 20.0f);
    EXPECT_FLOAT_EQ(r.rectPx.w, 100.0f);
    EXPECT_FLOAT_EQ(r.rectPx.h, 50.0f);
    EXPECT_EQ(r.color, Color(1, 2, 3, 4));
}

TEST(UIPhase1, LabelEmitsTextCommand)
{
    UI::UILabel label;
    label.setPositionPx(7.0f, 9.0f);
    label.setZ(2);
    label.setText("Hello");
    label.style().textColor  = Color(10, 20, 30, 40);
    label.style().textSizePx = 22;
    label.style().fontPath   = "SomeFont.ttf";

    UI::UIDrawList list;
    label.render(list);

    ASSERT_EQ(list.commands().size(), 1u);
    const auto& cmd = list.commands()[0];
    EXPECT_EQ(cmd.z, 2);

    ASSERT_TRUE(std::holds_alternative<UI::UIDrawText>(cmd.payload));
    const auto& t = std::get<UI::UIDrawText>(cmd.payload);
    EXPECT_FLOAT_EQ(t.positionPx.x, 7.0f);
    EXPECT_FLOAT_EQ(t.positionPx.y, 9.0f);
    EXPECT_EQ(t.text, "Hello");
    EXPECT_EQ(t.color, Color(10, 20, 30, 40));
    EXPECT_EQ(t.fontPath, "SomeFont.ttf");
    EXPECT_EQ(t.sizePx, 22u);
}

TEST(UIPhase1, RenderTraversesChildren)
{
    UI::UIContext ui;

    auto panel = std::make_unique<UI::UIPanel>();
    panel->setPositionPx(0.0f, 0.0f);
    panel->setSizePx(10.0f, 10.0f);
    panel->style().backgroundColor = Color(5, 6, 7, 8);

    auto label = std::make_unique<UI::UILabel>();
    label->setPositionPx(1.0f, 2.0f);
    label->setText("Child");

    panel->addChild(std::move(label));
    ui.root().addChild(std::move(panel));

    UI::UIDrawList list;
    ui.root().render(list);

    ASSERT_EQ(list.commands().size(), 2u);
    EXPECT_TRUE(std::holds_alternative<UI::UIDrawRect>(list.commands()[0].payload));
    EXPECT_TRUE(std::holds_alternative<UI::UIDrawText>(list.commands()[1].payload));
}
