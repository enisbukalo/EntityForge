#include <gtest/gtest.h>

#include <EntityManager.h>
#include <Registry.h>

namespace
{
struct DummyComponent
{
    int value = 0;

    DummyComponent() = default;
    explicit DummyComponent(int v) : value(v) {}
};
}

TEST(ComponentStoreDefensiveTest, HasReturnsFalseWhenDenseIndexOutOfBounds)
{
    EntityManager em;
    ComponentStore<DummyComponent> store;

    const Entity e1 = em.create();
    const Entity e2 = em.create();

    store.add(e1, 1);
    store.add(e2, 2);

    ASSERT_TRUE(store.has(e1));
    ASSERT_TRUE(store.has(e2));

    // Simulate stale/corrupt sparse mapping: shrink the dense arrays without
    // updating the sparse index map.
    store.entities().pop_back();

    EXPECT_TRUE(store.has(e1));
    EXPECT_FALSE(store.has(e2));

    store.entities().clear();
    EXPECT_FALSE(store.has(e1));
    EXPECT_FALSE(store.has(e2));
}
