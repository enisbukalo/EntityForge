#ifndef WORLD_H
#define WORLD_H

#include <cassert>
#include <typeindex>

#include "EventBus.h"
#include "Registry.h"

/**
 * @brief World is the orchestration point for entities and components
 *
 * Thin wrapper over Registry to make the public API explicit for callers.
 */
class World
{
public:
    class Components
    {
    public:
        explicit Components(World& world) : m_world(world) {}

        template <typename T, typename... Args>
        T* add(Entity e, Args&&... args)
        {
            m_world.assertAlive(e, "components.add");
            return m_world.m_registry.add<T>(e, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        void queueAdd(Entity e, Args&&... args)
        {
            m_world.assertAlive(e, "components.queueAdd");
            m_world.m_registry.queueAdd<T>(e, std::forward<Args>(args)...);
        }

        template <typename T>
        void remove(Entity e)
        {
            m_world.assertAlive(e, "components.remove");
            m_world.m_registry.remove<T>(e);
        }

        template <typename T>
        void queueRemove(Entity e)
        {
            m_world.assertAlive(e, "components.queueRemove");
            m_world.m_registry.queueRemove<T>(e);
        }

        template <typename T>
        void queueRemoveBatch(const std::vector<Entity>& entities)
        {
            for (Entity entity : entities)
            {
                m_world.assertAlive(entity, "components.queueRemoveBatch");
            }
            m_world.m_registry.queueRemoveBatch<T>(entities);
        }

        template <typename T>
        bool has(Entity e) const
        {
            return m_world.m_registry.has<T>(e);
        }

        template <typename T>
        T* get(Entity e)
        {
            m_world.assertAlive(e, "components.get");
            return m_world.m_registry.get<T>(e);
        }

        template <typename T>
        const T* get(Entity e) const
        {
            m_world.assertAlive(e, "components.get");
            return m_world.m_registry.get<T>(e);
        }

        template <typename T>
        T* tryGet(Entity e)
        {
            m_world.assertAlive(e, "components.tryGet");
            return m_world.m_registry.tryGet<T>(e);
        }

        template <typename T>
        const T* tryGet(Entity e) const
        {
            m_world.assertAlive(e, "components.tryGet");
            return m_world.m_registry.tryGet<T>(e);
        }

        template <typename T, typename Func>
        void each(Func&& fn)
        {
            m_world.m_registry.each<T>(std::forward<Func>(fn));
        }

        template <typename T, typename Func>
        void each(Func&& fn) const
        {
            m_world.m_registry.each<T>(std::forward<Func>(fn));
        }

        template <typename... ComponentTypes, typename Func>
        void view(Func&& fn)
        {
            m_world.m_registry.view<ComponentTypes...>(std::forward<Func>(fn));
        }

        template <typename... ComponentTypes, typename Func>
        void view(Func&& fn) const
        {
            m_world.m_registry.view<ComponentTypes...>(std::forward<Func>(fn));
        }

        template <typename... ComponentTypes, typename Func>
        void viewSorted(Func&& fn)
        {
            m_world.m_registry.viewSorted<ComponentTypes...>(std::forward<Func>(fn));
        }

        template <typename... ComponentTypes, typename Func, typename Compare>
        void viewSorted(Func&& fn, Compare&& compare)
        {
            m_world.m_registry.viewSorted<ComponentTypes...>(std::forward<Func>(fn), std::forward<Compare>(compare));
        }

        template <typename... ComponentTypes, typename Func>
        void viewSorted(Func&& fn) const
        {
            m_world.m_registry.viewSorted<ComponentTypes...>(std::forward<Func>(fn));
        }

        template <typename... ComponentTypes, typename Func, typename Compare>
        void viewSorted(Func&& fn, Compare&& compare) const
        {
            m_world.m_registry.viewSorted<ComponentTypes...>(std::forward<Func>(fn), std::forward<Compare>(compare));
        }

        template <typename A, typename B, typename Func>
        void view2(Func&& fn)
        {
            m_world.m_registry.view2<A, B>(std::forward<Func>(fn));
        }

        template <typename A, typename B, typename Func>
        void view2(Func&& fn) const
        {
            m_world.m_registry.view2<A, B>(std::forward<Func>(fn));
        }

        template <typename A, typename B, typename C, typename Func>
        void view3(Func&& fn)
        {
            m_world.m_registry.view3<A, B, C>(std::forward<Func>(fn));
        }

        template <typename A, typename B, typename C, typename Func>
        void view3(Func&& fn) const
        {
            m_world.m_registry.view3<A, B, C>(std::forward<Func>(fn));
        }

    private:
        World& m_world;
    };

    class ConstComponents
    {
    public:
        explicit ConstComponents(const World& world) : m_world(world) {}

        template <typename T>
        bool has(Entity e) const
        {
            return m_world.m_registry.has<T>(e);
        }

        template <typename T>
        const T* get(Entity e) const
        {
            m_world.assertAlive(e, "components.get");
            return m_world.m_registry.get<T>(e);
        }

        template <typename T>
        const T* tryGet(Entity e) const
        {
            m_world.assertAlive(e, "components.tryGet");
            return m_world.m_registry.tryGet<T>(e);
        }

        template <typename T, typename Func>
        void each(Func&& fn) const
        {
            m_world.m_registry.each<T>(std::forward<Func>(fn));
        }

        template <typename... ComponentTypes, typename Func>
        void view(Func&& fn) const
        {
            m_world.m_registry.view<ComponentTypes...>(std::forward<Func>(fn));
        }

        template <typename... ComponentTypes, typename Func>
        void viewSorted(Func&& fn) const
        {
            m_world.m_registry.viewSorted<ComponentTypes...>(std::forward<Func>(fn));
        }

        template <typename... ComponentTypes, typename Func, typename Compare>
        void viewSorted(Func&& fn, Compare&& compare) const
        {
            m_world.m_registry.viewSorted<ComponentTypes...>(std::forward<Func>(fn), std::forward<Compare>(compare));
        }

        template <typename A, typename B, typename Func>
        void view2(Func&& fn) const
        {
            m_world.m_registry.view2<A, B>(std::forward<Func>(fn));
        }

        template <typename A, typename B, typename C, typename Func>
        void view3(Func&& fn) const
        {
            m_world.m_registry.view3<A, B, C>(std::forward<Func>(fn));
        }

    private:
        const World& m_world;
    };

    World()  = default;
    ~World() = default;

    World(const World&)            = delete;
    World& operator=(const World&) = delete;

    Entity createEntity()
    {
        return m_registry.createEntity();
    }
    void destroyEntity(Entity e)
    {
        assertAlive(e, "destroyEntity");
        m_registry.destroy(e);
    }
    void queueDestroy(Entity e)
    {
        assertAlive(e, "queueDestroy");
        m_registry.queueDestroy(e);
    }
    void processDestroyQueue()
    {
        m_registry.processDestroyQueue();
    }
    void flushCommandBuffer()
    {
        m_registry.flushCommandBuffer();
    }
    size_t pendingDestroyCount() const
    {
        return m_registry.pendingDestroyCount();
    }
    bool isAlive(Entity e) const
    {
        return m_registry.isAlive(e);
    }

    Components components()
    {
        return Components(*this);
    }
    ConstComponents components() const
    {
        return ConstComponents(*this);
    }

    EventBus& events()
    {
        return m_events;
    }
    const EventBus& events() const
    {
        return m_events;
    }

    template <typename T, typename... Args>
    T* add(Entity e, Args&&... args)
    {
        assertAlive(e, "add");
        return m_registry.add<T>(e, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    void queueAdd(Entity e, Args&&... args)
    {
        assertAlive(e, "queueAdd");
        m_registry.queueAdd<T>(e, std::forward<Args>(args)...);
    }

    template <typename T>
    void remove(Entity e)
    {
        assertAlive(e, "remove");
        m_registry.remove<T>(e);
    }

    template <typename T>
    void queueRemove(Entity e)
    {
        assertAlive(e, "queueRemove");
        m_registry.queueRemove<T>(e);
    }

    template <typename... Components>
    Entity queueSpawn(Components&&... components)
    {
        return m_registry.queueSpawn(std::forward<Components>(components)...);
    }

    template <typename T>
    void queueRemoveBatch(const std::vector<Entity>& entities)
    {
        for (Entity entity : entities)
        {
            assertAlive(entity, "queueRemoveBatch");
        }
        m_registry.queueRemoveBatch<T>(entities);
    }

    void queueDestroyBatch(const std::vector<Entity>& entities)
    {
        for (Entity entity : entities)
        {
            assertAlive(entity, "queueDestroyBatch");
        }
        m_registry.queueDestroyBatch(entities);
    }

    template <typename T>
    bool has(Entity e) const
    {
        return m_registry.has<T>(e);
    }

    template <typename T>
    T* get(Entity e)
    {
        assertAlive(e, "get");
        return m_registry.get<T>(e);
    }

    template <typename T>
    const T* get(Entity e) const
    {
        assertAlive(e, "get");
        return m_registry.get<T>(e);
    }

    template <typename T>
    T* tryGet(Entity e)
    {
        assertAlive(e, "tryGet");
        return m_registry.tryGet<T>(e);
    }

    template <typename T>
    const T* tryGet(Entity e) const
    {
        assertAlive(e, "tryGet");
        return m_registry.tryGet<T>(e);
    }

    template <typename T, typename Func>
    void each(Func&& fn)
    {
        m_registry.each<T>(std::forward<Func>(fn));
    }

    template <typename T, typename Func>
    void each(Func&& fn) const
    {
        m_registry.each<T>(std::forward<Func>(fn));
    }

    template <typename... Components, typename Func>
    void view(Func&& fn)
    {
        m_registry.view<Components...>(std::forward<Func>(fn));
    }

    template <typename... Components, typename Func>
    void view(Func&& fn) const
    {
        m_registry.view<Components...>(std::forward<Func>(fn));
    }

    template <typename... Components, typename Func>
    void viewSorted(Func&& fn)
    {
        m_registry.viewSorted<Components...>(std::forward<Func>(fn));
    }

    template <typename... Components, typename Func, typename Compare>
    void viewSorted(Func&& fn, Compare&& compare)
    {
        m_registry.viewSorted<Components...>(std::forward<Func>(fn), std::forward<Compare>(compare));
    }

    template <typename... Components, typename Func>
    void viewSorted(Func&& fn) const
    {
        m_registry.viewSorted<Components...>(std::forward<Func>(fn));
    }

    template <typename... Components, typename Func, typename Compare>
    void viewSorted(Func&& fn, Compare&& compare) const
    {
        m_registry.viewSorted<Components...>(std::forward<Func>(fn), std::forward<Compare>(compare));
    }

    template <typename A, typename B, typename Func>
    void view2(Func&& fn)
    {
        m_registry.view2<A, B>(std::forward<Func>(fn));
    }

    template <typename A, typename B, typename Func>
    void view2(Func&& fn) const
    {
        m_registry.view2<A, B>(std::forward<Func>(fn));
    }

    template <typename A, typename B, typename C, typename Func>
    void view3(Func&& fn)
    {
        m_registry.view3<A, B, C>(std::forward<Func>(fn));
    }

    template <typename A, typename B, typename C, typename Func>
    void view3(Func&& fn) const
    {
        m_registry.view3<A, B, C>(std::forward<Func>(fn));
    }

    const std::vector<Entity>& getEntities() const
    {
        return m_registry.getEntities();
    }
    const std::vector<std::type_index>& getComposition(Entity e) const
    {
        return m_registry.getComposition(e);
    }

    void clear()
    {
        m_registry.clear();
    }

    template <typename T>
    void registerTypeName(const std::string& typeName)
    {
        m_registry.registerTypeName<T>(typeName);
    }

    template <typename T>
    std::string getTypeName() const
    {
        return m_registry.getTypeName<T>();
    }

    std::type_index getTypeFromName(const std::string& name) const
    {
        return m_registry.getTypeFromName(name);
    }

private:
    EventBus m_events;
    Registry m_registry;

    void assertAlive(Entity e, const char* action) const
    {
        (void)action;
        assert(e.isValid() && "World API received null entity");
        assert(m_registry.isAlive(e) && "World API received dead entity");
    }
};

#endif  // WORLD_H
