#include "ExampleHud.h"

#include <cmath>
#include <iomanip>
#include <sstream>

#include <Components.h>
#include <ObjectiveRegistry.h>
#include <SystemLocator.h>

#include <Color.h>
#include <GameEngine.h>

#include <UIContext.h>
#include <UILabel.h>
#include <UIPanel.h>
#include <UIVerticalLayout.h>

namespace
{

void applyHudLabelStyle(UI::UILabel& label)
{
    label.style().textColor  = Color::White;
    label.style().textSizePx = 18;
#if defined(_WIN32)
    label.style().fontPath = "C:/Windows/Fonts/arial.ttf";
#else
    label.style().fontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif
}

std::string objectiveStatusToString(Components::ObjectiveStatus status)
{
    switch (status)
    {
        case Components::ObjectiveStatus::Inactive:
            return "Inactive";
        case Components::ObjectiveStatus::InProgress:
            return "In Progress";
        case Components::ObjectiveStatus::Completed:
            return "Completed";
    }
    return "Unknown";
}

}  // namespace

namespace Example
{

ExampleHud ExampleHud::create(UI::UIContext& ui, GameEngine& engine, const std::vector<std::string>& objectiveIds)
{
    return create(ui, engine, objectiveIds, Config{});
}

ExampleHud ExampleHud::create(UI::UIContext& ui, GameEngine& engine, const std::vector<std::string>& objectiveIds, const Config& config)
{
    ExampleHud hud;

    // Boat speed label (top-left)
    {
        auto label       = std::make_unique<UI::UILabel>();
        hud.m_speedLabel = label.get();
        label->setPositionPx(config.marginPx, config.marginPx);
        label->setSizePx(420.0f, 24.0f);
        applyHudLabelStyle(*label);
        label->setText("Speed: 0.00 m/s");
        ui.root().addChild(std::move(label));
    }

    // Objectives panel (top-right)
    {
        auto panel = std::make_unique<UI::UIPanel>();
        panel->setAnchorsNormalized(1.0f, 0.0f, 1.0f, 0.0f);
        panel->setOffsetsPx(-(config.panelW + config.marginPx),
                            config.marginPx,
                            -config.marginPx,
                            config.marginPx + config.panelH);
        // Dark grey at ~25% opacity.
        panel->style().backgroundColor = Color(40, 40, 40, 64);

        auto vbox = std::make_unique<UI::UIVerticalLayout>();
        vbox->setPositionPx(10.0f, 10.0f);
        vbox->setSizePx(config.panelW - 20.0f, config.panelH - 20.0f);
        vbox->setSpacingPx(4.0f);
        vbox->setCrossAxisAlignment(UI::UIVerticalLayout::CrossAxisAlignment::Stretch);

        {
            auto label = std::make_unique<UI::UILabel>();
            label->setSizePx(config.panelW - 20.0f, 24.0f);
            applyHudLabelStyle(*label);
            label->setText("Objectives:");
            vbox->addChild(std::move(label));
        }

        hud.m_objectiveLines.reserve(objectiveIds.size());
        for (const auto& id : objectiveIds)
        {
            const auto* def   = engine.getObjectiveRegistry().find(id);
            std::string title = (def != nullptr && !def->title.empty()) ? def->title : id;

            std::vector<std::string> signalIds;
            std::int64_t             signalCount = 0;
            if (def != nullptr && def->progression.mode == Objectives::ProgressionMode::Signals && def->progression.signalCount > 1)
            {
                signalIds   = def->progression.signals;
                signalCount = def->progression.signalCount;
            }

            auto         label    = std::make_unique<UI::UILabel>();
            UI::UILabel* labelPtr = label.get();
            label->setSizePx(config.panelW - 20.0f, 20.0f);
            applyHudLabelStyle(*label);
            if (signalCount > 1)
            {
                label->setText("Inactive: " + title + " (0/" + std::to_string(signalCount) + ")");
            }
            else
            {
                label->setText("Inactive: " + title);
            }
            vbox->addChild(std::move(label));

            ObjectiveHudLine line;
            line.id          = std::string(id);
            line.title       = std::move(title);
            line.signalIds   = std::move(signalIds);
            line.signalCount = signalCount;
            line.label       = labelPtr;
            hud.m_objectiveLines.push_back(std::move(line));
        }

        panel->addChild(std::move(vbox));
        ui.root().addChild(std::move(panel));
    }

    return hud;
}

void ExampleHud::update(World& world, Entity boat, Entity objectiveState)
{
    if (m_speedLabel)
    {
        const b2Vec2 vel   = Systems::SystemLocator::physics().getLinearVelocity(boat);
        const float  speed = std::sqrt((vel.x * vel.x) + (vel.y * vel.y));

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "Speed: " << speed << " m/s";
        m_speedLabel->setText(ss.str());
    }

    const Components::CObjectives* obj = objectiveState.isValid()
                                             ? world.components().tryGet<Components::CObjectives>(objectiveState)
                                             : nullptr;

    for (auto& line : m_objectiveLines)
    {
        if (!line.label)
        {
            continue;
        }

        Components::ObjectiveStatus          status = Components::ObjectiveStatus::Inactive;
        const Components::ObjectiveInstance* inst   = nullptr;
        if (obj)
        {
            inst = obj->tryGetObjective(line.id);
            if (inst)
            {
                status = inst->status;
            }
        }

        std::string progress;
        if (inst != nullptr && line.signalCount > 1)
        {
            std::int64_t current = 0;
            for (const auto& sid : line.signalIds)
            {
                const auto it = inst->counters.find(sid);
                if (it != inst->counters.end())
                {
                    current += it->second;
                }
            }
            progress = " (" + std::to_string(current) + "/" + std::to_string(line.signalCount) + ")";
        }

        line.label->setText(objectiveStatusToString(status) + ": " + line.title + progress);
    }
}

}  // namespace Example
