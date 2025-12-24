#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <Entity.h>

class GameEngine;
class World;

namespace UI
{
class UIContext;
class UILabel;
}

namespace Example
{

class ExampleHud
{
public:
    struct Config
    {
        float marginPx = 20.0f;
        float panelW   = 520.0f;
        float panelH   = 200.0f;
    };

    ExampleHud() = default;

    static ExampleHud create(UI::UIContext& ui,
                             GameEngine& engine,
                             const std::vector<std::string>& objectiveIds);

    static ExampleHud create(UI::UIContext& ui,
                             GameEngine& engine,
                             const std::vector<std::string>& objectiveIds,
                             const Config& config);

    void update(World& world, Entity boat, Entity objectiveState);

private:
    struct ObjectiveHudLine
    {
        std::string  id;
        std::string  title;
        std::vector<std::string> signalIds;
        std::int64_t             signalCount{0};
        UI::UILabel* label{};
    };

    UI::UILabel*                  m_speedLabel = nullptr;
    std::vector<ObjectiveHudLine> m_objectiveLines;
};

}  // namespace Example
