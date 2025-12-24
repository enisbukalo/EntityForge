#include <gtest/gtest.h>

#include <FileUtilities.h>

#include <ObjectiveRegistry.h>

#include <filesystem>
#include <fstream>

namespace
{
std::filesystem::path makeTempDir(const std::string& name)
{
    auto dir = std::filesystem::temp_directory_path() / ("gameengine_objectives_" + name);
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    std::filesystem::create_directories(dir, ec);
    return dir;
}

void writeJson(const std::filesystem::path& file, const std::string& content)
{
    Internal::FileUtilities::writeFile(file.string(), content);
}

}  // namespace

TEST(ObjectiveRegistry, LoadsValidDefinitions)
{
    const auto dir = makeTempDir("valid");

    writeJson(dir / "a.json", R"({
        "id": "quest.a",
        "title": "A",
        "description": "desc A"
    })");

    writeJson(dir / "b.json", R"({
        "id": "quest.b",
        "title": "B",
        "description": "desc B",
        "prerequisites": ["quest.a"],
        "onComplete": "GrantReward"
    })");

    Objectives::ObjectiveRegistry registry;
    std::vector<std::string>      errors;
    ASSERT_TRUE(registry.loadFromDirectory(dir, &errors));
    EXPECT_TRUE(errors.empty());
    EXPECT_EQ(registry.size(), 2u);

    const auto* b = registry.find("quest.b");
    ASSERT_NE(b, nullptr);
    ASSERT_EQ(b->prerequisites.size(), 1u);
    EXPECT_EQ(b->prerequisites[0], "quest.a");
    EXPECT_EQ(b->onComplete, "GrantReward");

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(ObjectiveRegistry, FailsOnMissingPrerequisite)
{
    const auto dir = makeTempDir("missing_prereq");

    writeJson(dir / "b.json", R"({
        "id": "quest.b",
        "title": "B",
        "description": "desc B",
        "prerequisites": ["quest.a"]
    })");

    Objectives::ObjectiveRegistry registry;
    std::vector<std::string>      errors;
    EXPECT_FALSE(registry.loadFromDirectory(dir, &errors));
    EXPECT_FALSE(errors.empty());

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(ObjectiveRegistry, FailsOnCycle)
{
    const auto dir = makeTempDir("cycle");

    writeJson(dir / "a.json", R"({
        "id": "quest.a",
        "title": "A",
        "description": "desc A",
        "prerequisites": ["quest.b"]
    })");

    writeJson(dir / "b.json", R"({
        "id": "quest.b",
        "title": "B",
        "description": "desc B",
        "prerequisites": ["quest.a"]
    })");

    Objectives::ObjectiveRegistry registry;
    std::vector<std::string>      errors;
    EXPECT_FALSE(registry.loadFromDirectory(dir, &errors));
    EXPECT_FALSE(errors.empty());

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
}

TEST(ObjectiveRegistryTest, RejectsUnknownProgressionMode)
{
        const auto dir = std::filesystem::temp_directory_path() / "objective_registry_unknown_progression";
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);

        std::ofstream f(dir / "bad.json", std::ios::binary);
        ASSERT_TRUE(static_cast<bool>(f));
        f << R"({
    \"id\": \"quest.bad\",
    \"title\": \"Bad\",
    \"description\": \"Bad\",
    \"progression\": { \"mode\": \"not-a-mode\" }
})";

        Objectives::ObjectiveRegistry registry;
        std::vector<std::string> errors;
        EXPECT_FALSE(registry.loadFromDirectory(dir, &errors));
        EXPECT_FALSE(errors.empty());

        std::filesystem::remove_all(dir, ec);
}

TEST(ObjectiveRegistryTest, RejectsTriggerRuleWithMissingKey)
{
        const auto dir = std::filesystem::temp_directory_path() / "objective_registry_bad_trigger_key";
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        std::filesystem::create_directories(dir, ec);

        std::ofstream f(dir / "bad.json", std::ios::binary);
        ASSERT_TRUE(static_cast<bool>(f));
        f << R"({
    \"id\": \"quest.bad_trigger\",
    \"title\": \"Bad Trigger\",
    \"description\": \"Bad Trigger\",
    \"progression\": {
        \"mode\": \"triggers\",
        \"triggers\": [
            {\"type\": \"Enter\", \"triggerName\": \"T\", \"action\": \"IncrementCounter\"}
        ]
    }
})";

        Objectives::ObjectiveRegistry registry;
        std::vector<std::string> errors;
        EXPECT_FALSE(registry.loadFromDirectory(dir, &errors));
        EXPECT_FALSE(errors.empty());

        std::filesystem::remove_all(dir, ec);
}
