#include <gtest/gtest.h>

#include <cmath>

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

    panel.layout(UI::UIRect{0.0f, 0.0f, 800.0f, 600.0f});

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

    label.layout(UI::UIRect{0.0f, 0.0f, 800.0f, 600.0f});

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

    ui.layout(UI::UIRect{0.0f, 0.0f, 800.0f, 600.0f});

    UI::UIDrawList list;
    ui.root().render(list);

    ASSERT_EQ(list.commands().size(), 2u);
    EXPECT_TRUE(std::holds_alternative<UI::UIDrawRect>(list.commands()[0].payload));
    EXPECT_TRUE(std::holds_alternative<UI::UIDrawText>(list.commands()[1].payload));
}

TEST(UIPhase2, CenterAnchorWithOffsetsComputesExpectedRect)
{
    UI::UIPanel panel;
    panel.setAnchorsNormalized(0.5f, 0.5f, 0.5f, 0.5f);
    panel.setOffsetsPx(-50.0f, -25.0f, 50.0f, 25.0f);

    panel.layout(UI::UIRect{0.0f, 0.0f, 200.0f, 100.0f});
    const UI::UIRect r = panel.rectPx();

    EXPECT_FLOAT_EQ(r.x, 50.0f);
    EXPECT_FLOAT_EQ(r.y, 25.0f);
    EXPECT_FLOAT_EQ(r.w, 100.0f);
    EXPECT_FLOAT_EQ(r.h, 50.0f);
}

TEST(UIPhase2, StretchAnchorsWithPaddingComputesExpectedRect)
{
    UI::UIPanel panel;
    panel.setAnchorsNormalized(0.0f, 0.0f, 1.0f, 1.0f);
    panel.setOffsetsPx(10.0f, 20.0f, -10.0f, -20.0f);

    panel.layout(UI::UIRect{0.0f, 0.0f, 300.0f, 200.0f});
    const UI::UIRect r = panel.rectPx();

    EXPECT_FLOAT_EQ(r.x, 10.0f);
    EXPECT_FLOAT_EQ(r.y, 20.0f);
    EXPECT_FLOAT_EQ(r.w, 280.0f);
    EXPECT_FLOAT_EQ(r.h, 160.0f);
}

TEST(UIPhase2, ParentResizeRecomputesStretchLayout)
{
    UI::UIPanel panel;
    panel.setAnchorsNormalized(0.0f, 0.0f, 1.0f, 1.0f);
    panel.setOffsetsPx(10.0f, 20.0f, -10.0f, -20.0f);

    panel.layout(UI::UIRect{0.0f, 0.0f, 100.0f, 100.0f});
    const UI::UIRect r1 = panel.rectPx();

    panel.layout(UI::UIRect{0.0f, 0.0f, 200.0f, 200.0f});
    const UI::UIRect r2 = panel.rectPx();

    EXPECT_FLOAT_EQ(r1.x, 10.0f);
    EXPECT_FLOAT_EQ(r1.y, 20.0f);
    EXPECT_FLOAT_EQ(r1.w, 80.0f);
    EXPECT_FLOAT_EQ(r1.h, 60.0f);

    EXPECT_FLOAT_EQ(r2.x, 10.0f);
    EXPECT_FLOAT_EQ(r2.y, 20.0f);
    EXPECT_FLOAT_EQ(r2.w, 180.0f);
    EXPECT_FLOAT_EQ(r2.h, 160.0f);
}

TEST(UIPhase2, PivotMovesRectRelativeToAnchor)
{
    UI::UIPanel panel;
    panel.setAnchorsNormalized(0.5f, 0.5f, 0.5f, 0.5f);
    panel.setPivotNormalized(0.5f, 0.5f);
    panel.setOffsetsPx(0.0f, 0.0f, 10.0f, 20.0f);

    panel.layout(UI::UIRect{0.0f, 0.0f, 100.0f, 100.0f});
    const UI::UIRect r = panel.rectPx();

    EXPECT_FLOAT_EQ(r.x, 45.0f);
    EXPECT_FLOAT_EQ(r.y, 40.0f);
    EXPECT_FLOAT_EQ(r.w, 10.0f);
    EXPECT_FLOAT_EQ(r.h, 20.0f);
}

TEST(UIPhase2, OffsetChangeRecomputesWithSameParentRect)
{
    UI::UIPanel panel;
    panel.setAnchorsNormalized(0.5f, 0.5f, 0.5f, 0.5f);
    panel.setPivotNormalized(0.0f, 0.0f);
    panel.setOffsetsPx(-10.0f, -10.0f, 10.0f, 10.0f);

    const UI::UIRect parent{0.0f, 0.0f, 100.0f, 100.0f};
    panel.layout(parent);
    const UI::UIRect r1 = panel.rectPx();

    panel.setOffsetsPx(-20.0f, -5.0f, 20.0f, 5.0f);
    panel.layout(parent);
    const UI::UIRect r2 = panel.rectPx();

    EXPECT_FLOAT_EQ(r1.x, 40.0f);
    EXPECT_FLOAT_EQ(r1.y, 40.0f);
    EXPECT_FLOAT_EQ(r1.w, 20.0f);
    EXPECT_FLOAT_EQ(r1.h, 20.0f);

    EXPECT_FLOAT_EQ(r2.x, 30.0f);
    EXPECT_FLOAT_EQ(r2.y, 45.0f);
    EXPECT_FLOAT_EQ(r2.w, 40.0f);
    EXPECT_FLOAT_EQ(r2.h, 10.0f);
}

TEST(UIPhase2, PivotChangeRecomputesWithSameOffsets)
{
    UI::UIPanel panel;
    panel.setAnchorsNormalized(0.5f, 0.5f, 0.5f, 0.5f);
    panel.setOffsetsPx(0.0f, 0.0f, 10.0f, 20.0f);

    const UI::UIRect parent{0.0f, 0.0f, 100.0f, 100.0f};

    panel.setPivotNormalized(0.0f, 0.0f);
    panel.layout(parent);
    const UI::UIRect r1 = panel.rectPx();

    panel.setPivotNormalized(1.0f, 1.0f);
    panel.layout(parent);
    const UI::UIRect r2 = panel.rectPx();

    EXPECT_FLOAT_EQ(r1.x, 50.0f);
    EXPECT_FLOAT_EQ(r1.y, 50.0f);
    EXPECT_FLOAT_EQ(r1.w, 10.0f);
    EXPECT_FLOAT_EQ(r1.h, 20.0f);

    EXPECT_FLOAT_EQ(r2.x, 40.0f);
    EXPECT_FLOAT_EQ(r2.y, 30.0f);
    EXPECT_FLOAT_EQ(r2.w, 10.0f);
    EXPECT_FLOAT_EQ(r2.h, 20.0f);
}

TEST(UIPhase2, PivotDoesNotAffectStretchedAnchors)
{
    UI::UIPanel panel;
    panel.setAnchorsNormalized(0.0f, 0.0f, 1.0f, 1.0f);
    panel.setOffsetsPx(10.0f, 20.0f, -10.0f, -20.0f);

    const UI::UIRect parent{0.0f, 0.0f, 300.0f, 200.0f};

    panel.setPivotNormalized(0.0f, 0.0f);
    panel.layout(parent);
    const UI::UIRect r1 = panel.rectPx();

    panel.setPivotNormalized(1.0f, 1.0f);
    panel.layout(parent);
    const UI::UIRect r2 = panel.rectPx();

    EXPECT_FLOAT_EQ(r1.x, 10.0f);
    EXPECT_FLOAT_EQ(r1.y, 20.0f);
    EXPECT_FLOAT_EQ(r1.w, 280.0f);
    EXPECT_FLOAT_EQ(r1.h, 160.0f);

    EXPECT_FLOAT_EQ(r2.x, 10.0f);
    EXPECT_FLOAT_EQ(r2.y, 20.0f);
    EXPECT_FLOAT_EQ(r2.w, 280.0f);
    EXPECT_FLOAT_EQ(r2.h, 160.0f);
}

TEST(UIPhase2, MixedAnchorsPivotAffectsOnlyPointAxis)
{
    UI::UIPanel panel;
    // Stretch horizontally, point-anchor vertically (center).
    panel.setAnchorsNormalized(0.0f, 0.5f, 1.0f, 0.5f);
    panel.setOffsetsPx(10.0f, 0.0f, -10.0f, 20.0f);
    panel.setPivotNormalized(1.0f, 0.5f);

    panel.layout(UI::UIRect{0.0f, 0.0f, 200.0f, 100.0f});
    const UI::UIRect r = panel.rectPx();

    EXPECT_FLOAT_EQ(r.x, 10.0f);
    EXPECT_FLOAT_EQ(r.w, 180.0f);
    EXPECT_FLOAT_EQ(r.y, 40.0f);
    EXPECT_FLOAT_EQ(r.h, 20.0f);
}

TEST(UIPhase2, AnchorChangeRecomputesWithSameParentRect)
{
    UI::UIPanel panel;
    const UI::UIRect parent{0.0f, 0.0f, 200.0f, 100.0f};

    panel.setAnchorsNormalized(0.0f, 0.0f, 0.0f, 0.0f);
    panel.setPivotNormalized(0.0f, 0.0f);
    panel.setOffsetsPx(10.0f, 20.0f, 110.0f, 70.0f);

    panel.layout(parent);
    const UI::UIRect r1 = panel.rectPx();

    panel.setAnchorsNormalized(0.0f, 0.0f, 1.0f, 1.0f);
    panel.setOffsetsPx(5.0f, 6.0f, -7.0f, -8.0f);

    panel.layout(parent);
    const UI::UIRect r2 = panel.rectPx();

    EXPECT_FLOAT_EQ(r1.x, 10.0f);
    EXPECT_FLOAT_EQ(r1.y, 20.0f);
    EXPECT_FLOAT_EQ(r1.w, 100.0f);
    EXPECT_FLOAT_EQ(r1.h, 50.0f);

    EXPECT_FLOAT_EQ(r2.x, 5.0f);
    EXPECT_FLOAT_EQ(r2.y, 6.0f);
    EXPECT_FLOAT_EQ(r2.w, 188.0f);
    EXPECT_FLOAT_EQ(r2.h, 86.0f);
}

TEST(UIPhase2, ChildLayoutUsesComputedParentRect)
{
    // Parent is placed in absolute (Phase 1-compatible) mode.
    auto parent = std::make_unique<UI::UIPanel>();
    parent->setPositionPx(10.0f, 20.0f);
    parent->setSizePx(100.0f, 50.0f);

    // Child stretches within parent with padding margins.
    auto child = std::make_unique<UI::UIPanel>();
    child->setAnchorsNormalized(0.0f, 0.0f, 1.0f, 1.0f);
    child->setOffsetsPx(5.0f, 6.0f, -7.0f, -8.0f);

    UI::UIElement& childRef = parent->addChild(std::move(child));

    parent->layout(UI::UIRect{0.0f, 0.0f, 400.0f, 300.0f});

    const UI::UIRect parentRect = parent->rectPx();
    const UI::UIRect childRect  = childRef.rectPx();

    EXPECT_FLOAT_EQ(parentRect.x, 10.0f);
    EXPECT_FLOAT_EQ(parentRect.y, 20.0f);
    EXPECT_FLOAT_EQ(parentRect.w, 100.0f);
    EXPECT_FLOAT_EQ(parentRect.h, 50.0f);

    EXPECT_FLOAT_EQ(childRect.x, 15.0f);
    EXPECT_FLOAT_EQ(childRect.y, 26.0f);
    EXPECT_FLOAT_EQ(childRect.w, 88.0f);
    EXPECT_FLOAT_EQ(childRect.h, 36.0f);
}

TEST(UIPhase2, ChildRecomputesWhenParentChanges)
{
    auto parent = std::make_unique<UI::UIPanel>();
    parent->setAnchorsNormalized(0.5f, 0.5f, 0.5f, 0.5f);
    parent->setOffsetsPx(-50.0f, -25.0f, 50.0f, 25.0f);

    auto child = std::make_unique<UI::UIPanel>();
    child->setAnchorsNormalized(0.0f, 0.0f, 1.0f, 1.0f);
    child->setOffsetsPx(0.0f, 0.0f, 0.0f, 0.0f);

    UI::UIElement& childRef = parent->addChild(std::move(child));
    const UI::UIRect viewport{0.0f, 0.0f, 200.0f, 100.0f};

    parent->layout(viewport);
    const UI::UIRect r1 = childRef.rectPx();

    // Move/resize the parent via offsets; child should track computed parent rect.
    parent->setOffsetsPx(-40.0f, -20.0f, 40.0f, 20.0f);
    parent->layout(viewport);
    const UI::UIRect r2 = childRef.rectPx();

    EXPECT_FLOAT_EQ(r1.x, 50.0f);
    EXPECT_FLOAT_EQ(r1.y, 25.0f);
    EXPECT_FLOAT_EQ(r1.w, 100.0f);
    EXPECT_FLOAT_EQ(r1.h, 50.0f);

    EXPECT_FLOAT_EQ(r2.x, 60.0f);
    EXPECT_FLOAT_EQ(r2.y, 30.0f);
    EXPECT_FLOAT_EQ(r2.w, 80.0f);
    EXPECT_FLOAT_EQ(r2.h, 40.0f);
}

TEST(UIPhase2, ZeroSizedParentRectProducesFiniteRect)
{
    UI::UIPanel panel;
    panel.setAnchorsNormalized(0.5f, 0.5f, 0.5f, 0.5f);
    panel.setPivotNormalized(0.5f, 0.5f);
    panel.setOffsetsPx(0.0f, 0.0f, 10.0f, 20.0f);

    panel.layout(UI::UIRect{10.0f, 20.0f, 0.0f, 0.0f});
    const UI::UIRect r = panel.rectPx();

    EXPECT_TRUE(std::isfinite(r.x));
    EXPECT_TRUE(std::isfinite(r.y));
    EXPECT_TRUE(std::isfinite(r.w));
    EXPECT_TRUE(std::isfinite(r.h));

    EXPECT_FLOAT_EQ(r.x, 5.0f);
    EXPECT_FLOAT_EQ(r.y, 10.0f);
    EXPECT_FLOAT_EQ(r.w, 10.0f);
    EXPECT_FLOAT_EQ(r.h, 20.0f);
}

TEST(UIPhase2, NegativeSizeOffsetsArePreserved)
{
    UI::UIPanel panel;
    panel.setAnchorsNormalized(0.0f, 0.0f, 0.0f, 0.0f);
    panel.setPivotNormalized(0.0f, 0.0f);
    panel.setOffsetsPx(10.0f, 10.0f, 0.0f, 0.0f);

    panel.layout(UI::UIRect{0.0f, 0.0f, 100.0f, 100.0f});
    const UI::UIRect r = panel.rectPx();

    EXPECT_FLOAT_EQ(r.x, 10.0f);
    EXPECT_FLOAT_EQ(r.y, 10.0f);
    EXPECT_FLOAT_EQ(r.w, -10.0f);
    EXPECT_FLOAT_EQ(r.h, -10.0f);
}
