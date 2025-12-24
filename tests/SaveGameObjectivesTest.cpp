#include <gtest/gtest.h>

#include <SaveGame.h>

#include <ExecutablePaths.h>
#include <FileUtilities.h>
#include <World.h>

#include <Components.h>
#include <components/CObjectives.h>

#include <filesystem>

namespace
{
std::filesystem::path resolveSaveFilePath(const std::string& slotFilename)
{
    const auto saveDir = Internal::ExecutablePaths::resolveRelativeToExecutableDir("saved_games");
    return saveDir / slotFilename;
}

Entity findEntityByName(const World& world, const std::string& name)
{
    Entity found = Entity::null();
    world.view<Components::CName>(
        [&](Entity e, const Components::CName& n)
        {
            if (n.name == name)
            {
                found = e;
            }
        });
    return found;
}

}  // namespace

TEST(SaveGameObjectives, RoundTripObjectivesComponent)
{
    const std::string slot = "savegame_objectives_round_trip";
    const auto        path = resolveSaveFilePath(slot + ".json");

    std::error_code ec;
    std::filesystem::remove(path, ec);

    World world;

    Entity player = world.createEntity();
    world.add<Components::CName>(player, Components::CName{"Player"});

    Components::CObjectives objectives;
    {
        objectives.managerCounters["coins"] = 5;
        objectives.managerFlags["story.started"] = true;

        auto& o        = objectives.getOrAddObjective("quest.find_key");
        o.title        = "Find the Key";
        o.description  = "Pick up the rusty key.";
        o.status       = Components::ObjectiveStatus::Completed;
        o.rewardGranted = true;
        o.prerequisites = {"quest.talk_to_npc"};
        o.counters["keys"] = 1;
        o.onComplete   = "GrantKeyReward";

        auto& prereq = objectives.getOrAddObjective("quest.talk_to_npc");
        prereq.title = "Talk to the NPC";
        prereq.status = Components::ObjectiveStatus::Completed;
    }

    world.add<Components::CObjectives>(player, objectives);

    ASSERT_TRUE(Systems::SaveGame::saveWorld(world, slot));
    ASSERT_TRUE(std::filesystem::exists(path));

    World loaded;
    ASSERT_TRUE(Systems::SaveGame::loadWorld(loaded, slot, Systems::LoadMode::ReplaceWorld));

    const Entity loadedPlayer = findEntityByName(loaded, "Player");
    ASSERT_TRUE(loadedPlayer.isValid());

    const auto* loadedObjectives = loaded.get<Components::CObjectives>(loadedPlayer);
    ASSERT_TRUE(loadedObjectives != nullptr);

    const auto* q = loadedObjectives->tryGetObjective("quest.find_key");
    ASSERT_TRUE(q != nullptr);

    EXPECT_EQ(q->title, "Find the Key");
    EXPECT_EQ(q->description, "Pick up the rusty key.");
    EXPECT_EQ(q->status, Components::ObjectiveStatus::Completed);
    EXPECT_TRUE(q->rewardGranted);
    ASSERT_EQ(q->prerequisites.size(), 1u);
    EXPECT_EQ(q->prerequisites[0], "quest.talk_to_npc");
    EXPECT_EQ(q->onComplete, "GrantKeyReward");

    auto it = q->counters.find("keys");
    ASSERT_TRUE(it != q->counters.end());
    EXPECT_EQ(it->second, 1);

    EXPECT_EQ(loadedObjectives->getManagerCounter("coins"), 5);
    EXPECT_TRUE(loadedObjectives->isManagerFlagSet("story.started"));

    // Reward idempotency: already granted should not grant again.
    Components::CObjectives copy = *loadedObjectives;
    EXPECT_FALSE(copy.tryGrantObjectiveReward("quest.find_key"));

    // Fresh completed objective should grant exactly once.
    copy.setObjectiveStatus("quest.temp", Components::ObjectiveStatus::Completed);
    EXPECT_TRUE(copy.tryGrantObjectiveReward("quest.temp"));
    EXPECT_FALSE(copy.tryGrantObjectiveReward("quest.temp"));

    std::filesystem::remove(path, ec);
    std::filesystem::remove(path.parent_path(), ec);
}
