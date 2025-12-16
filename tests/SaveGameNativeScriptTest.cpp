#include <gtest/gtest.h>

#include <SaveGame.h>

#include <CNativeScript.h>
#include <ExecutablePaths.h>
#include <FileUtilities.h>
#include <ISerializableScript.h>
#include <ScriptTypeRegistry.h>
#include <SScript.h>
#include <World.h>

#include <filesystem>
#include <string>

namespace
{

std::filesystem::path resolveSaveFilePath(const std::string& slotFilename)
{
    const auto saveDir = Internal::ExecutablePaths::resolveRelativeToExecutableDir("saved_games");
    return saveDir / slotFilename;
}

Entity findFirstScriptedEntity(const World& world)
{
    Entity found = Entity::null();
    world.view<Components::CNativeScript>([&](Entity e, const Components::CNativeScript& /*s*/)
    {
        found = e;
    });
    return found;
}

class NativeScriptSerializableTestScript final : public Components::INativeScript, public Components::ISerializableScript
{
public:
    static constexpr const char* kScriptName = "NativeScriptSerializableTestScript";

    int         hp    = 0;
    float       speed = 0.0f;
    std::string tag;

    bool onCreateSawLoadedFields = false;

    void onCreate(Entity /*self*/, World& /*world*/) override
    {
        onCreateSawLoadedFields = (hp == 123 && speed == 4.5f && tag == "hello");
    }

    void onUpdate(float /*deltaTime*/, Entity /*self*/, World& /*world*/) override {}

    void serializeFields(Serialization::ScriptFieldWriter& out) const override
    {
        out.setInt("hp", hp);
        out.setFloat("speed", speed);
        out.setString("tag", tag);
    }

    void deserializeFields(const Serialization::ScriptFieldReader& in) override
    {
        if (auto v = in.getInt("hp"))
        {
            hp = static_cast<int>(*v);
        }
        if (auto v = in.getFloat("speed"))
        {
            speed = static_cast<float>(*v);
        }
        if (auto v = in.getString("tag"))
        {
            tag = *v;
        }
    }
};

}  // namespace

TEST(SaveGameNativeScript, RoundTripTypeAndFields)
{
    Serialization::ScriptTypeRegistry::instance().registerScript<NativeScriptSerializableTestScript>(NativeScriptSerializableTestScript::kScriptName);

    const std::string slot = "savegame_native_script_round_trip";
    const auto        path = resolveSaveFilePath(slot + ".json");

    std::error_code ec;
    std::filesystem::remove(path, ec);

    World world;

    Entity e = world.createEntity();
    auto*  ns = world.components().add<Components::CNativeScript>(e);
    ns->bind<NativeScriptSerializableTestScript>();

    auto* before = dynamic_cast<NativeScriptSerializableTestScript*>(ns->instance.get());
    ASSERT_NE(before, nullptr);

    before->hp    = 123;
    before->speed = 4.5f;
    before->tag   = "hello";

    ASSERT_TRUE(Systems::SaveGame::saveWorld(world, slot));
    ASSERT_TRUE(std::filesystem::exists(path));

    World loaded;
    ASSERT_TRUE(Systems::SaveGame::loadWorld(loaded, slot, Systems::LoadMode::ReplaceWorld));

    const Entity loadedEntity = findFirstScriptedEntity(loaded);
    ASSERT_TRUE(loadedEntity.isValid());

    auto* loadedNs = loaded.components().tryGet<Components::CNativeScript>(loadedEntity);
    ASSERT_NE(loadedNs, nullptr);

    EXPECT_EQ(loadedNs->scriptTypeName, NativeScriptSerializableTestScript::kScriptName);
    ASSERT_TRUE(loadedNs->instance != nullptr);

    auto* after = dynamic_cast<NativeScriptSerializableTestScript*>(loadedNs->instance.get());
    ASSERT_NE(after, nullptr);

    EXPECT_EQ(after->hp, 123);
    EXPECT_FLOAT_EQ(after->speed, 4.5f);
    EXPECT_EQ(after->tag, "hello");

    // Verify fields are applied before the first post-load update (onCreate sees loaded values).
    Systems::SScript scriptSystem;
    scriptSystem.update(0.016f, loaded);

    loadedNs = loaded.components().tryGet<Components::CNativeScript>(loadedEntity);
    ASSERT_NE(loadedNs, nullptr);
    after = dynamic_cast<NativeScriptSerializableTestScript*>(loadedNs->instance.get());
    ASSERT_NE(after, nullptr);
    EXPECT_TRUE(after->onCreateSawLoadedFields);

    std::filesystem::remove(path, ec);
    std::filesystem::remove(path.parent_path(), ec);
}

TEST(SaveGameNativeScript, MissingFieldsDoesNotCrashLoad)
{
    Serialization::ScriptTypeRegistry::instance().registerScript<NativeScriptSerializableTestScript>(NativeScriptSerializableTestScript::kScriptName);

    const std::string slot = "savegame_native_script_missing_fields";
    const auto        path = resolveSaveFilePath(slot + ".json");

    std::error_code ec;
    std::filesystem::remove(path, ec);

    // Hand-write a minimal save file that has a script type but no 'fields' object.
    {
        std::filesystem::create_directories(path.parent_path(), ec);

        const std::string content =
            "{\n"
            "  \"format\": \"GameEngineSave\",\n"
            "  \"version\": 1,\n"
            "  \"engine\": { \"name\": \"GameEngine\", \"semver\": \"0.1.0\" },\n"
            "  \"createdUtc\": \"2025-12-16T00:00:00Z\",\n"
            "  \"entities\": [\n"
            "    {\n"
            "      \"id\": \"0\",\n"
            "      \"components\": [\n"
            "        { \"type\": \"CNativeScript\", \"data\": { \"scriptType\": \"NativeScriptSerializableTestScript\" } }\n"
            "      ]\n"
            "    }\n"
            "  ]\n"
            "}\n";

        Internal::FileUtilities::writeFile(path.string(), content);
    }

    World loaded;
    ASSERT_TRUE(Systems::SaveGame::loadWorld(loaded, slot, Systems::LoadMode::ReplaceWorld));

    const Entity loadedEntity = findFirstScriptedEntity(loaded);
    ASSERT_TRUE(loadedEntity.isValid());

    auto* loadedNs = loaded.components().tryGet<Components::CNativeScript>(loadedEntity);
    ASSERT_NE(loadedNs, nullptr);
    ASSERT_TRUE(loadedNs->instance != nullptr);

    auto* after = dynamic_cast<NativeScriptSerializableTestScript*>(loadedNs->instance.get());
    ASSERT_NE(after, nullptr);

    // Defaults remain (no crash, no fields applied)
    EXPECT_EQ(after->hp, 0);
    EXPECT_FLOAT_EQ(after->speed, 0.0f);
    EXPECT_TRUE(after->tag.empty());

    std::filesystem::remove(path, ec);
    std::filesystem::remove(path.parent_path(), ec);
}
