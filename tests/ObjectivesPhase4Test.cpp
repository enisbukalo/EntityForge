#include <gtest/gtest.h>

#include <SObjectives.h>

#include <World.h>

#include <CName.h>
#include <CObjectives.h>
#include <ObjectiveEvents.h>
#include <ObjectiveRegistry.h>
#include <TriggerEvents.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
std::filesystem::path makeTempDir(std::string_view leaf)
{
    static int counter = 0;

    std::error_code ec;
    auto dir = std::filesystem::temp_directory_path(ec);
    if (ec)
    {
        // Fallback to current directory; tests still run in CI containers.
        dir = std::filesystem::current_path();
    }

    std::filesystem::path out = dir / (std::string(leaf) + "_" + std::to_string(++counter));
    std::filesystem::create_directories(out, ec);
    return out;
}

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::ofstream f(path, std::ios::binary);
    ASSERT_TRUE(static_cast<bool>(f));
    f << text;
}

Components::CObjectives* getObjectives(World& world, Entity e)
{
    return world.components().tryGet<Components::CObjectives>(e);
}

}  // namespace

TEST(ObjectivesPhase4, ActivationGatingPrereqAndConditionsAreEnforced)
{
    const auto dir = makeTempDir("objectives_phase4_activate");

    // Two objectives:
    // - quest.prereq: already completed
    // - quest.main: gated by prereq + FlagSet + CounterAtLeast
    const std::string json = R"(
[
  {
    "id": "quest.prereq",
    "title": "Prereq",
    "description": "Prereq desc",
    "progression": { "mode": "targeted" }
  },
  {
    "id": "quest.main",
    "title": "Main Quest",
    "description": "Main desc",
    "prerequisites": ["quest.prereq"],
    "activationConditions": [
      {"type": "FlagSet", "flag": "story.started"},
      {"type": "CounterAtLeast", "key": "coins", "value": 3}
    ],
    "progression": { "mode": "signals", "signals": ["sig.complete_main"] }
  }
]
)";

    writeTextFile(dir / "defs.json", json);

    Objectives::ObjectiveRegistry registry;
    std::vector<std::string>     errors;
    ASSERT_TRUE(registry.loadFromDirectory(dir, &errors)) << (errors.empty() ? "" : errors[0]);

    World world;

    // Store objectives on a single player entity.
    const Entity player = world.createEntity();
    {
        Components::CObjectives c;
        auto& prereq = c.getOrAddObjective("quest.prereq");
        prereq.status = Components::ObjectiveStatus::Completed;
        world.add<Components::CObjectives>(player, std::move(c));
    }

    Systems::SObjectives sys(&registry);

    // First update registers subscriptions.
    sys.update(0.0f, world);

    // Activation request is delivered during pump and applied in the next tick.
    world.events().emit<Objectives::ObjectiveActivate>(Objectives::ObjectiveActivate{"quest.main"});
    world.events().pump(EventStage::PreFlush, world);

    // Not applied until next update().
    {
        auto* c = getObjectives(world, player);
        ASSERT_NE(c, nullptr);
        auto* main = c->tryGetObjective("quest.main");
        EXPECT_TRUE(main == nullptr || main->status == Components::ObjectiveStatus::Inactive);
    }

    sys.update(0.0f, world);

    // Still gated (conditions not satisfied).
    {
        auto* c = getObjectives(world, player);
        ASSERT_NE(c, nullptr);
        auto* main = c->tryGetObjective("quest.main");
        EXPECT_TRUE(main == nullptr || main->status == Components::ObjectiveStatus::Inactive);
    }

    // Satisfy conditions via manager events.
    world.events().emit<Objectives::ObjectiveManagerFlagSet>(Objectives::ObjectiveManagerFlagSet{"story.started", true});
    world.events().emit<Objectives::ObjectiveManagerCounterIncrement>(Objectives::ObjectiveManagerCounterIncrement{"coins", 3});
    world.events().pump(EventStage::PreFlush, world);
    sys.update(0.0f, world);

    // Try activation again.
    world.events().emit<Objectives::ObjectiveActivate>(Objectives::ObjectiveActivate{"quest.main"});
    world.events().pump(EventStage::PreFlush, world);
    sys.update(0.0f, world);

    auto* c = getObjectives(world, player);
    ASSERT_NE(c, nullptr);

    const auto* main = c->tryGetObjective("quest.main");
    ASSERT_NE(main, nullptr);
    EXPECT_EQ(main->status, Components::ObjectiveStatus::InProgress);
    EXPECT_EQ(main->title, "Main Quest");
    EXPECT_EQ(main->description, "Main desc");
}

TEST(ObjectivesPhase4, SignalProgressionCompletesObjectiveNextTick)
{
    const auto dir = makeTempDir("objectives_phase4_signal");

    const std::string json = R"(
{
  "id": "quest.signal",
  "title": "Signal Quest",
  "description": "Wait for signal",
  "progression": { "mode": "signals", "signals": ["sig.finish"] }
}

TEST(ObjectivesPhase4, SignalProgressionWithCountRequiresMultipleSignals)
{
  const auto dir = makeTempDir("objectives_phase4_signal_count");

  const std::string json = R"(
{
  "id": "quest.signal_count",
  "title": "Signal Count Quest",
  "description": "Requires 3 signals",
  "progression": { "mode": "signals", "count": 3, "signals": ["sig.hit"] }
}
)";

  writeTextFile(dir / "signal_count.json", json);

  Objectives::ObjectiveRegistry registry;
  std::vector<std::string>     errors;
  ASSERT_TRUE(registry.loadFromDirectory(dir, &errors)) << (errors.empty() ? "" : errors[0]);

  World world;
  const Entity player = world.createEntity();
  world.add<Components::CObjectives>(player, Components::CObjectives{});

  Systems::SObjectives sys(&registry);
  sys.update(0.0f, world);

  // Activate.
  world.events().emit<Objectives::ObjectiveActivate>(Objectives::ObjectiveActivate{"quest.signal_count"});
  world.events().pump(EventStage::PreFlush, world);
  sys.update(0.0f, world);

  // First signal -> still in progress after update.
  world.events().emit<Objectives::ObjectiveSignal>(Objectives::ObjectiveSignal{"sig.hit"});
  world.events().pump(EventStage::PreFlush, world);
  sys.update(0.0f, world);
  {
    auto* c = getObjectives(world, player);
    ASSERT_NE(c, nullptr);
    const auto* inst = c->tryGetObjective("quest.signal_count");
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->status, Components::ObjectiveStatus::InProgress);
  }

  // Second signal -> still in progress.
  world.events().emit<Objectives::ObjectiveSignal>(Objectives::ObjectiveSignal{"sig.hit"});
  world.events().pump(EventStage::PreFlush, world);
  sys.update(0.0f, world);
  {
    auto* c = getObjectives(world, player);
    ASSERT_NE(c, nullptr);
    const auto* inst = c->tryGetObjective("quest.signal_count");
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->status, Components::ObjectiveStatus::InProgress);
  }

  // Third signal -> completes.
  world.events().emit<Objectives::ObjectiveSignal>(Objectives::ObjectiveSignal{"sig.hit"});
  world.events().pump(EventStage::PreFlush, world);
  sys.update(0.0f, world);
  {
    auto* c = getObjectives(world, player);
    ASSERT_NE(c, nullptr);
    const auto* inst = c->tryGetObjective("quest.signal_count");
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->status, Components::ObjectiveStatus::Completed);
    EXPECT_TRUE(inst->rewardGranted);
  }
}
)";

    writeTextFile(dir / "signal.json", json);

    Objectives::ObjectiveRegistry registry;
    std::vector<std::string>     errors;
    ASSERT_TRUE(registry.loadFromDirectory(dir, &errors)) << (errors.empty() ? "" : errors[0]);

    World world;
    const Entity player = world.createEntity();
    world.add<Components::CObjectives>(player, Components::CObjectives{});

    Systems::SObjectives sys(&registry);
    sys.update(0.0f, world);

    // Activate.
    world.events().emit<Objectives::ObjectiveActivate>(Objectives::ObjectiveActivate{"quest.signal"});
    world.events().pump(EventStage::PreFlush, world);
    sys.update(0.0f, world);

    // Emit an unrelated signal (should not complete).
    world.events().emit<Objectives::ObjectiveSignal>(Objectives::ObjectiveSignal{"sig.other"});
    world.events().pump(EventStage::PreFlush, world);
    sys.update(0.0f, world);

    {
        auto* c = getObjectives(world, player);
        ASSERT_NE(c, nullptr);
        const auto* inst = c->tryGetObjective("quest.signal");
        ASSERT_NE(inst, nullptr);
        EXPECT_EQ(inst->status, Components::ObjectiveStatus::InProgress);
    }

    // Emit matching signal. It should be queued during pump and applied in next update().
    world.events().emit<Objectives::ObjectiveSignal>(Objectives::ObjectiveSignal{"sig.finish"});
    world.events().pump(EventStage::PreFlush, world);

    {
        auto* c = getObjectives(world, player);
        ASSERT_NE(c, nullptr);
        const auto* inst = c->tryGetObjective("quest.signal");
        ASSERT_NE(inst, nullptr);
        EXPECT_EQ(inst->status, Components::ObjectiveStatus::InProgress);
    }

    sys.update(0.0f, world);

    {
        auto* c = getObjectives(world, player);
        ASSERT_NE(c, nullptr);
        const auto* inst = c->tryGetObjective("quest.signal");
        ASSERT_NE(inst, nullptr);
        EXPECT_EQ(inst->status, Components::ObjectiveStatus::Completed);
        EXPECT_TRUE(inst->rewardGranted);
    }
}

TEST(ObjectivesPhase4, TriggerEnterCompletesObjectiveByTriggerName)
{
    const auto dir = makeTempDir("objectives_phase4_trigger");

    const std::string json = R"(
{
  "id": "quest.enter_checkpoint",
  "title": "Enter checkpoint",
  "description": "Walk into the checkpoint",
  "progression": {
    "mode": "triggers",
    "triggers": [
      {"type": "Enter", "triggerName": "Checkpoint", "action": "Complete"}
    ]
  }
}
)";

    writeTextFile(dir / "trigger.json", json);

    Objectives::ObjectiveRegistry registry;
    std::vector<std::string>     errors;
    ASSERT_TRUE(registry.loadFromDirectory(dir, &errors)) << (errors.empty() ? "" : errors[0]);

    World world;

    const Entity player = world.createEntity();
    world.add<Components::CObjectives>(player, Components::CObjectives{});

    // Trigger entity with a name (MVP trigger reference strategy).
    const Entity trigger = world.createEntity();
    world.add<Components::CName>(trigger, Components::CName{"Checkpoint"});

    const Entity other = world.createEntity();

    Systems::SObjectives sys(&registry);
    sys.update(0.0f, world);

    // Activate.
    world.events().emit<Objectives::ObjectiveActivate>(Objectives::ObjectiveActivate{"quest.enter_checkpoint"});
    world.events().pump(EventStage::PreFlush, world);
    sys.update(0.0f, world);

    // Emit trigger enter; complete should happen on next update().
    world.events().emit<Physics::TriggerEnter>(Physics::TriggerEnter{trigger, other});
    world.events().pump(EventStage::PostFlush, world);

    {
        auto* c = getObjectives(world, player);
        ASSERT_NE(c, nullptr);
        const auto* inst = c->tryGetObjective("quest.enter_checkpoint");
        ASSERT_NE(inst, nullptr);
        EXPECT_EQ(inst->status, Components::ObjectiveStatus::InProgress);
    }

    sys.update(0.0f, world);

    {
        auto* c = getObjectives(world, player);
        ASSERT_NE(c, nullptr);
        const auto* inst = c->tryGetObjective("quest.enter_checkpoint");
        ASSERT_NE(inst, nullptr);
        EXPECT_EQ(inst->status, Components::ObjectiveStatus::Completed);
        EXPECT_TRUE(inst->rewardGranted);
    }
}

TEST(ObjectivesPhase4, RegistryRejectsUnknownActivationConditionType)
{
    const auto dir = makeTempDir("objectives_phase4_bad_condition");

    const std::string json = R"(
{
  "id": "quest.bad",
  "title": "Bad",
  "description": "Bad",
  "activationConditions": [
    {"type": "NotARealType", "flag": "x"}
  ]
}
)";

    writeTextFile(dir / "bad.json", json);

    Objectives::ObjectiveRegistry registry;
    std::vector<std::string>     errors;
    EXPECT_FALSE(registry.loadFromDirectory(dir, &errors));
    EXPECT_FALSE(errors.empty());
}

TEST(ObjectivesPhase4, RegistryRejectsMalformedTriggerRules)
{
    const auto dir = makeTempDir("objectives_phase4_bad_trigger_rule");

    // Missing triggerName.
    const std::string json = R"(
{
  "id": "quest.bad_trigger",
  "title": "Bad Trigger",
  "description": "Bad Trigger",
  "progression": {
    "mode": "triggers",
    "triggers": [
      {"type": "Enter", "action": "Complete"}
    ]
  }
}
)";

    writeTextFile(dir / "bad_trigger.json", json);

    Objectives::ObjectiveRegistry registry;
    std::vector<std::string>     errors;
    EXPECT_FALSE(registry.loadFromDirectory(dir, &errors));
    EXPECT_FALSE(errors.empty());
}
