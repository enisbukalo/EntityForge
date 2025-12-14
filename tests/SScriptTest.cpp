#include <gtest/gtest.h>

#include <CNativeScript.h>
#include <World.h>

#include <SScript.h>

namespace
{

class SpawnsManyScripts final : public Components::INativeScript
{
public:
    explicit SpawnsManyScripts(int* createCount) : m_createCount(createCount) {}

    void onCreate(Entity /*self*/, World& world) override
    {
        if (m_createCount)
        {
            (*m_createCount)++;
        }

        for (int i = 0; i < 256; ++i)
        {
            Entity e = world.createEntity();
            auto*  s = world.components().add<Components::CNativeScript>(e);
            s->bind<SpawnsManyScripts>(nullptr);
        }
    }

    void onUpdate(float /*deltaTime*/, Entity /*self*/, World& /*world*/) override {}

private:
    int* m_createCount = nullptr;
};

}  // namespace

TEST(SScriptTest, OnCreateRunsOnceWhenOnCreateSpawnsScripts)
{
    World world;
    Systems::SScript scriptSystem;

    int onCreateCount = 0;

    Entity spawner = world.createEntity();
    auto*  script  = world.components().add<Components::CNativeScript>(spawner);
    script->bind<SpawnsManyScripts>(&onCreateCount);

    scriptSystem.update(0.016f, world);
    scriptSystem.update(0.016f, world);

    EXPECT_EQ(onCreateCount, 1);
}
