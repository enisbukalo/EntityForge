#ifndef REGISTRY_H
#define REGISTRY_H

#include <Entity.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cstdio>

#include <EntityManager.h>
#include "Logger.h"

/**
 * @brief Type-erased interface for component storage
 *
 * Allows Registry to manage multiple ComponentStore<T> instances
 * in a type-safe container using runtime polymorphism.
 */
class IComponentStore
{
public:
    virtual ~IComponentStore() = default;

    /**
     * @brief Removes component for the specified entity (if it exists)
     */
    virtual void remove(Entity entity) = 0;

    /**
     * @brief Checks if entity has this component type
     */
    virtual bool has(Entity entity) const = 0;

    /**
     * @brief Gets count of components in this store
     */
    virtual size_t size() const = 0;

    /**
     * @brief Gets the type name for serialization
     */
    virtual std::string getTypeName() const = 0;
};

/**
 * @brief Dense component storage with sparse entity->index mapping (vector-based)
 *
 * Components are stored densely; a sparse vector maps entity index -> dense index.
 * Removal uses swap-and-pop while keeping the sparse mapping up to date.
 *
 * @tparam T Component type
 */
template <typename T>
class ComponentStore : public IComponentStore
{
public:
    ComponentStore() = default;

    /**
     * @brief Adds or replaces a component for an entity
     */
    template <typename... Args>
    T& add(Entity entity, Args&&... args)
    {
        ensureSparse(entity.index);

        if (has(entity))
        {
            auto denseIndex        = m_sparse[entity.index];
            m_dense[denseIndex]    = T(std::forward<Args>(args)...);
            m_entities[denseIndex] = entity;
            return m_dense[denseIndex];
        }

        auto denseIndex        = static_cast<uint32_t>(m_dense.size());
        m_sparse[entity.index] = denseIndex;
        m_entities.push_back(entity);
        m_dense.emplace_back(std::forward<Args>(args)...);
        return m_dense.back();
    }

    /**
     * @brief Removes component for an entity using swap-and-pop
     */
    void remove(Entity entity) override
    {
        if (!has(entity))
        {
            return;
        }

        auto denseIndex = m_sparse[entity.index];
        auto lastIndex  = static_cast<uint32_t>(m_dense.size() - 1);

        if (denseIndex != lastIndex)
        {
            std::swap(m_dense[denseIndex], m_dense[lastIndex]);
            std::swap(m_entities[denseIndex], m_entities[lastIndex]);
            m_sparse[m_entities[denseIndex].index] = denseIndex;
        }

        m_sparse[entity.index] = kInvalid;
        m_dense.pop_back();
        m_entities.pop_back();
    }

    /**
     * @brief Checks if entity has this component
     */
    bool has(Entity entity) const override
    {
        return entity.index < m_sparse.size() && m_sparse[entity.index] != kInvalid && m_entities[m_sparse[entity.index]] == entity;
    }

    /**
     * @brief Gets component for entity (asserts if not present)
     */
    T& get(Entity entity)
    {
        assert(has(entity) && "Entity does not have this component");
        return m_dense[m_sparse[entity.index]];
    }

    /**
     * @brief Gets component for entity (const version)
     */
    const T& get(Entity entity) const
    {
        assert(has(entity) && "Entity does not have this component");
        return m_dense[m_sparse[entity.index]];
    }

    /**
     * @brief Tries to get component, returns nullptr if not present
     */
    T* tryGet(Entity entity)
    {
        return has(entity) ? &m_dense[m_sparse[entity.index]] : nullptr;
    }

    /**
     * @brief Tries to get component (const version)
     */
    const T* tryGet(Entity entity) const
    {
        return has(entity) ? &m_dense[m_sparse[entity.index]] : nullptr;
    }

    /**
     * @brief Iterates over all components, calling fn(Entity, T&) for each
     */
    template <typename Func>
    void each(Func&& fn)
    {
        for (size_t i = 0; i < m_dense.size(); ++i)
        {
            fn(m_entities[i], m_dense[i]);
        }
    }

    /**
     * @brief Iterates over all components (const version)
     */
    template <typename Func>
    void each(Func&& fn) const
    {
        for (size_t i = 0; i < m_dense.size(); ++i)
        {
            fn(m_entities[i], m_dense[i]);
        }
    }

    /**
     * @brief Gets number of components in storage
     */
    size_t size() const override
    {
        return m_dense.size();
    }

    /**
     * @brief Gets the type name for serialization
     */
    std::string getTypeName() const override
    {
        return typeid(T).name();
    }

    std::vector<T>& components()
    {
        return m_dense;
    }
    const std::vector<T>& components() const
    {
        return m_dense;
    }

    std::vector<Entity>& entities()
    {
        return m_entities;
    }
    const std::vector<Entity>& entities() const
    {
        return m_entities;
    }

private:
    void ensureSparse(uint32_t index)
    {
        if (index >= m_sparse.size())
        {
            m_sparse.resize(index + 1, kInvalid);
        }
    }

    static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();

    std::vector<uint32_t> m_sparse;    ///< Entity index -> dense index mapping
    std::vector<Entity>   m_entities;  ///< Parallel array of entity handles
    std::vector<T>        m_dense;     ///< Densely-packed component array
};

/**
 * @brief Central registry for entity and component management
 *
 * The Registry is the single source of truth for all entities and their components.
 * It owns per-type component stores and provides O(1) expected add/remove/has/get.
 * GameEngine owns one Registry instance (single world/scene model).
 */
class Registry
{
public:
    Registry()  = default;
    ~Registry() = default;

    // Non-copyable, non-movable
    Registry(const Registry&)            = delete;
    Registry& operator=(const Registry&) = delete;

    /**
     * @brief Creates a new entity
     * @return The created entity ID
     */
    Entity createEntity()
    {
        Entity entity = m_entityManager.create();
        if (entity.isValid())
        {
            ensureCompositionEntry(entity);
            m_entityComposition[entity.index].clear();
            m_entities.push_back(entity);
        }
        return entity;
    }

    /**
     * @brief Destroys an entity and removes all its components
     * @param entity Entity to destroy
     */
    void destroy(Entity entity)
    {
        if (!m_entityManager.isAlive(entity))
            return;

        bool removedViaComposition = false;
        if (entity.index < m_entityComposition.size())
        {
            auto& types = m_entityComposition[entity.index];
            if (!types.empty())
            {
                removedViaComposition = true;
                for (auto typeIdx : types)
                {
                    if (auto* store = getStore(typeIdx))
                    {
                        store->remove(entity);
                    }
                }
            }
            types.clear();
        }

        if (!removedViaComposition)
        {
            for (auto& [typeIndex, store] : m_componentStores)
            {
                store->remove(entity);
            }
        }

        auto it = std::find(m_entities.begin(), m_entities.end(), entity);
        if (it != m_entities.end())
        {
            m_entities.erase(it);
        }

        m_entityManager.destroy(entity);
    }

    /**
     * @brief Queues an entity for destruction at a safe point (typically end-of-frame)
     */
    void queueDestroy(Entity entity)
    {
        if (!ensureAlive(entity, "queueDestroy"))
        {
            return;
        }

        auto it = std::find(m_deferredDestroy.begin(), m_deferredDestroy.end(), entity);
        if (it == m_deferredDestroy.end())
        {
            m_deferredDestroy.push_back(entity);
        }
    }

    /**
     * @brief Processes all queued destructions
     */
    void processDestroyQueue()
    {
        if (m_deferredDestroy.empty())
        {
            return;
        }

        for (Entity entity : m_deferredDestroy)
        {
            destroy(entity);
        }
        m_deferredDestroy.clear();
    }

    void flushCommandBuffer()
    {
        if (m_commandBuffer.empty() && m_deferredDestroy.empty())
        {
            return;
        }

        auto commands = std::move(m_commandBuffer);
        m_commandBuffer.clear();

        for (const auto& command : commands)
        {
            command();
        }

        processDestroyQueue();
    }

    /**
     * @brief Gets the number of pending destroy requests
     */
    size_t pendingDestroyCount() const
    {
        return m_deferredDestroy.size();
    }

    /**
     * @brief Checks if an entity handle is alive
     */
    bool isAlive(Entity entity) const
    {
        return m_entityManager.isAlive(entity);
    }

    /**
     * @brief Adds a component to an entity
     * @tparam T Component type
     * @param entity Entity to add component to
     * @param args Arguments forwarded to component constructor
     * @return Reference to the created component
     */
    template <typename T, typename... Args>
    T* add(Entity entity, Args&&... args)
    {
        assert(m_entityManager.isAlive(entity) && "Cannot add component to dead or null entity");
        if (!ensureAlive(entity, "add"))
        {
            return nullptr;
        }
        auto& store     = getOrCreateStore<T>();
        T&    component = store.add(entity, std::forward<Args>(args)...);

        trackComponentAdd(entity, std::type_index(typeid(T)));

        LOG_DEBUG("Component added: {} @ E{}:G{}", typeid(T).name(), entity.index, entity.generation);

        // Automatically wire owner on components that expose setOwner (most current components)

        return &component;
    }

    /**
     * @brief Removes a component from an entity
     * @tparam T Component type
     * @param entity Entity to remove component from
     */
    template <typename T>
    void remove(Entity entity)
    {
        if (!ensureAlive(entity, "remove"))
        {
            return;
        }
        auto* store = getStore<T>();
        if (store)
        {
            LOG_DEBUG("Component removed: {} @ E{}:G{}", typeid(T).name(), entity.index, entity.generation);
            store->remove(entity);
            trackComponentRemove(entity, std::type_index(typeid(T)));
        }
    }

    template <typename T, typename... Args>
    void queueAdd(Entity entity, Args&&... args)
    {
        if (!ensureAlive(entity, "queueAdd"))
        {
            return;
        }

        auto argsTuple = std::make_shared<std::tuple<std::decay_t<Args>...>>(std::forward<Args>(args)...);
        m_commandBuffer.emplace_back(
            [this, entity, argsTuple]()
            {
                if (!m_entityManager.isAlive(entity))
                {
                    logDead("queueAdd/execute", entity);
                    return;
                }

                std::apply([&](auto&... unpacked) { this->add<T>(entity, std::move(unpacked)...); }, *argsTuple);
            });
    }

    template <typename T>
    void queueRemove(Entity entity)
    {
        if (!ensureAlive(entity, "queueRemove"))
        {
            return;
        }

        m_commandBuffer.emplace_back(
            [this, entity]()
            {
                if (!m_entityManager.isAlive(entity))
                {
                    logDead("queueRemove/execute", entity);
                    return;
                }
                this->remove<T>(entity);
            });
    }

    template <typename... Components>
    Entity queueSpawn(Components&&... components)
    {
        Entity entity = createEntity();
        (queueAdd<std::decay_t<Components>>(entity, std::forward<Components>(components)), ...);
        return entity;
    }

    template <typename T>
    void queueRemoveBatch(const std::vector<Entity>& entities)
    {
        for (Entity entity : entities)
        {
            queueRemove<T>(entity);
        }
    }

    void queueDestroyBatch(const std::vector<Entity>& entities)
    {
        for (Entity entity : entities)
        {
            queueDestroy(entity);
        }
    }

    /**
     * @brief Checks if entity has a component
     * @tparam T Component type
     * @param entity Entity to check
     * @return true if component exists
     */
    template <typename T>
    bool has(Entity entity) const
    {
        if (!m_entityManager.isAlive(entity))
            return false;

        const auto typeIdx = std::type_index(typeid(T));
        if (!compositionHas(entity, typeIdx))
        {
            return false;
        }

        const auto* store = getStore<T>();
        return store && store->has(entity);
    }

    /**
     * @brief Gets a component (asserts if not present)
     * @tparam T Component type
     * @param entity Entity to get component from
     * @return Reference to component
     */
    template <typename T>
    T* get(Entity entity)
    {
        assert(m_entityManager.isAlive(entity) && "Cannot get component of dead or null entity");
        if (!ensureAlive(entity, "get"))
        {
            return nullptr;
        }
        auto* store = getStore<T>();
        return store ? store->tryGet(entity) : nullptr;
    }

    /**
     * @brief Gets a component (const version)
     */
    template <typename T>
    const T* get(Entity entity) const
    {
        assert(m_entityManager.isAlive(entity) && "Cannot get component of dead or null entity");
        if (!ensureAlive(entity, "get"))
        {
            return nullptr;
        }
        const auto* store = getStore<T>();
        return store ? store->tryGet(entity) : nullptr;
    }

    /**
     * @brief Tries to get a component, returns nullptr if not present
     * @tparam T Component type
     * @param entity Entity to get component from
     * @return Pointer to component or nullptr
     */
    template <typename T>
    T* tryGet(Entity entity)
    {
        if (!m_entityManager.isAlive(entity))
            return nullptr;
        auto* store = getStore<T>();
        return store ? store->tryGet(entity) : nullptr;
    }

    /**
     * @brief Tries to get a component (const version)
     */
    template <typename T>
    const T* tryGet(Entity entity) const
    {
        if (!m_entityManager.isAlive(entity))
            return nullptr;
        const auto* store = getStore<T>();
        return store ? store->tryGet(entity) : nullptr;
    }

    /**
     * @brief Iterates over all entities with component T
     * @tparam T Component type
     * @param fn Callback function(EntityId, T&)
     */
    template <typename T, typename Func>
    void each(Func&& fn)
    {
        auto* store = getStore<T>();
        if (store)
        {
            store->each(
                [&](Entity entity, T& component)
                {
                    if (!ensureAlive(entity, "each"))
                    {
                        return;
                    }
                    fn(entity, component);
                });
        }
    }

    /**
     * @brief Iterates over all entities with component T (const version)
     */
    template <typename T, typename Func>
    void each(Func&& fn) const
    {
        const auto* store = getStore<T>();
        if (store)
        {
            store->each(
                [&](Entity entity, const T& component)
                {
                    if (!ensureAlive(entity, "each"))
                    {
                        return;
                    }
                    fn(entity, component);
                });
        }
    }

    /**
     * @brief Iterate entities that have all specified component types using the smallest backing store.
     */
    template <typename... Components, typename Func>
    void view(Func&& fn)
    {
        static_assert(sizeof...(Components) > 0, "Registry::view requires at least one component");

        auto stores  = std::tuple<ComponentStore<Components>*...>{getStore<Components>()...};
        bool anyNull = false;
        std::apply([&](auto*... store) { ((anyNull = anyNull || (store == nullptr)), ...); }, stores);
        if (anyNull)
        {
            return;
        }

        constexpr size_t kCount       = sizeof...(Components);
        const auto       sizes        = std::apply([](auto*... store)
                                      { return std::array<size_t, kCount>{static_cast<size_t>(store->size())...}; },
                                      stores);
        const size_t     primaryIndex = static_cast<size_t>(
            std::distance(sizes.begin(), std::min_element(sizes.begin(), sizes.end())));

        auto hasAll = [&](Entity entity)
        {
            bool allPresent = true;
            std::apply([&](auto*... store) { ((allPresent = allPresent && store->has(entity)), ...); }, stores);
            return allPresent;
        };

        auto dispatch = [&](auto primaryConst)
        {
            constexpr size_t P            = decltype(primaryConst)::value;
            auto*            primaryStore = std::get<P>(stores);
            auto&            entities     = primaryStore->entities();
            auto&            components   = primaryStore->components();

            for (size_t i = 0; i < components.size(); ++i)
            {
                Entity entity = entities[i];
                if (!ensureAlive(entity, "view"))
                {
                    continue;
                }
                if (!hasAll(entity))
                {
                    continue;
                }

                callView<P>(fn, stores, components, entity, i, std::make_index_sequence<kCount>{});
            }
        };

        dispatchByIndex(primaryIndex, dispatch, std::make_index_sequence<kCount>{});
    }

    template <typename... Components, typename Func>
    void view(Func&& fn) const
    {
        static_assert(sizeof...(Components) > 0, "Registry::view requires at least one component");

        auto stores  = std::tuple<const ComponentStore<Components>*...>{getStore<Components>()...};
        bool anyNull = false;
        std::apply([&](const auto*... store) { ((anyNull = anyNull || (store == nullptr)), ...); }, stores);
        if (anyNull)
        {
            return;
        }

        constexpr size_t kCount       = sizeof...(Components);
        const auto       sizes        = std::apply([](const auto*... store)
                                      { return std::array<size_t, kCount>{static_cast<size_t>(store->size())...}; },
                                      stores);
        const size_t     primaryIndex = static_cast<size_t>(
            std::distance(sizes.begin(), std::min_element(sizes.begin(), sizes.end())));

        auto hasAll = [&](Entity entity)
        {
            bool allPresent = true;
            std::apply([&](const auto*... store) { ((allPresent = allPresent && store->has(entity)), ...); }, stores);
            return allPresent;
        };

        auto dispatch = [&](auto primaryConst)
        {
            constexpr size_t P            = decltype(primaryConst)::value;
            const auto*      primaryStore = std::get<P>(stores);
            const auto&      entities     = primaryStore->entities();
            const auto&      components   = primaryStore->components();

            for (size_t i = 0; i < components.size(); ++i)
            {
                Entity entity = entities[i];
                if (!ensureAlive(entity, "view"))
                {
                    continue;
                }
                if (!hasAll(entity))
                {
                    continue;
                }

                callView<P>(fn, stores, components, entity, i, std::make_index_sequence<kCount>{});
            }
        };

        dispatchByIndex(primaryIndex, dispatch, std::make_index_sequence<kCount>{});
    }

    template <typename... Components, typename Func>
    void viewSorted(Func&& fn)
    {
        viewSorted<Components...>(std::forward<Func>(fn), [](Entity a, Entity b) { return a < b; });
    }

    template <typename... Components, typename Func, typename Compare>
    void viewSorted(Func&& fn, Compare&& compare)
    {
        static_assert(sizeof...(Components) > 0, "Registry::viewSorted requires at least one component");

        using Item = std::tuple<Entity, Components*...>;
        std::vector<Item> items;

        view<Components...>([&](Entity entity, Components&... comps) { items.emplace_back(entity, &comps...); });

        if (items.empty())
        {
            return;
        }

        std::stable_sort(items.begin(),
                         items.end(),
                         [&](const Item& lhs, const Item& rhs) { return compare(std::get<0>(lhs), std::get<0>(rhs)); });

        for (const auto& item : items)
        {
            std::apply([&](Entity entity, Components*... comps) { fn(entity, *comps...); }, item);
        }
    }

    template <typename... Components, typename Func>
    void viewSorted(Func&& fn) const
    {
        viewSorted<Components...>(std::forward<Func>(fn), [](Entity a, Entity b) { return a < b; });
    }

    template <typename... Components, typename Func, typename Compare>
    void viewSorted(Func&& fn, Compare&& compare) const
    {
        static_assert(sizeof...(Components) > 0, "Registry::viewSorted requires at least one component");

        using Item = std::tuple<Entity, const Components*...>;
        std::vector<Item> items;

        view<Components...>([&](Entity entity, const Components&... comps) { items.emplace_back(entity, &comps...); });

        if (items.empty())
        {
            return;
        }

        std::stable_sort(items.begin(),
                         items.end(),
                         [&](const Item& lhs, const Item& rhs) { return compare(std::get<0>(lhs), std::get<0>(rhs)); });

        for (const auto& item : items)
        {
            std::apply([&](Entity entity, const Components*... comps) { fn(entity, *comps...); }, item);
        }
    }

    template <typename A, typename B, typename Func>
    void view2(Func&& fn)
    {
        view<A, B>(std::forward<Func>(fn));
    }

    template <typename A, typename B, typename Func>
    void view2(Func&& fn) const
    {
        view<A, B>(std::forward<Func>(fn));
    }

    template <typename A, typename B, typename C, typename Func>
    void view3(Func&& fn)
    {
        view<A, B, C>(std::forward<Func>(fn));
    }

    template <typename A, typename B, typename C, typename Func>
    void view3(Func&& fn) const
    {
        view<A, B, C>(std::forward<Func>(fn));
    }

    /**
     * @brief Gets all entities in the registry
     */
    const std::vector<Entity>& getEntities() const
    {
        return m_entities;
    }

    /**
     * @brief Gets the tracked component types for an entity
     */
    const std::vector<std::type_index>& getComposition(Entity entity) const
    {
        static const std::vector<std::type_index> kEmptyComposition;
        if (entity.index >= m_entityComposition.size())
        {
            return kEmptyComposition;
        }

        return m_entityComposition[entity.index];
    }

    /**
     * @brief Clears all entities and components
     */
    void clear()
    {
        m_componentStores.clear();
        m_entities.clear();
        m_entityManager.clear();
        m_deferredDestroy.clear();
        m_entityComposition.clear();
    }

    /**
     * @brief Registers a stable type name for serialization
     * @tparam T Component type
     * @param typeName Stable string name for this type
     */
    template <typename T>
    void registerTypeName(const std::string& typeName)
    {
        assert(!typeName.empty() && "Component type name must not be empty");
        std::type_index typeIdx(typeid(T));
        auto            existingByType = m_typeNames.find(typeIdx);
        if (existingByType != m_typeNames.end())
        {
            if (existingByType->second != typeName)
            {
                logTypeNameMismatch(existingByType->second, typeName);
            }
            return;
        }

        auto existingByName = m_nameToType.find(typeName);
        if (existingByName != m_nameToType.end())
        {
            logTypeNameCollision(typeName);
            return;
        }
        m_typeNames.emplace(typeIdx, typeName);
        m_nameToType.emplace(typeName, typeIdx);
    }

    /**
     * @brief Gets the stable type name for a component type
     */
    template <typename T>
    std::string getTypeName() const
    {
        auto it = m_typeNames.find(std::type_index(typeid(T)));
        return (it != m_typeNames.end()) ? it->second : typeid(T).name();
    }

    /**
     * @brief Gets type_index from a stable type name
     */
    std::type_index getTypeFromName(const std::string& typeName) const
    {
        auto it = m_nameToType.find(typeName);
        if (it == m_nameToType.end())
        {
            logTypeLookupFailure(typeName);
            return std::type_index(typeid(void));
        }
        return it->second;
    }

private:
    /**
     * @brief Gets or creates a component store for type T
     */
    template <typename T>
    ComponentStore<T>& getOrCreateStore()
    {
        std::type_index typeIdx(typeid(T));
        auto            it = m_componentStores.find(typeIdx);

        if (it == m_componentStores.end())
        {
            auto  store                = std::make_unique<ComponentStore<T>>();
            auto* storePtr             = store.get();
            m_componentStores[typeIdx] = std::move(store);
            return *storePtr;
        }

        return *static_cast<ComponentStore<T>*>(it->second.get());
    }

    /**
     * @brief Gets an existing component store for type T
     */
    template <typename T>
    ComponentStore<T>* getStore()
    {
        auto it = m_componentStores.find(std::type_index(typeid(T)));
        return (it != m_componentStores.end()) ? static_cast<ComponentStore<T>*>(it->second.get()) : nullptr;
    }

    /**
     * @brief Gets an existing component store (const version)
     */
    template <typename T>
    const ComponentStore<T>* getStore() const
    {
        auto it = m_componentStores.find(std::type_index(typeid(T)));
        return (it != m_componentStores.end()) ? static_cast<const ComponentStore<T>*>(it->second.get()) : nullptr;
    }

    IComponentStore* getStore(std::type_index typeIdx)
    {
        auto it = m_componentStores.find(typeIdx);
        return (it != m_componentStores.end()) ? it->second.get() : nullptr;
    }

    const IComponentStore* getStore(std::type_index typeIdx) const
    {
        auto it = m_componentStores.find(typeIdx);
        return (it != m_componentStores.end()) ? it->second.get() : nullptr;
    }

    template <typename Func, size_t... Is>
    static void dispatchByIndex(size_t index, Func&& fn, std::index_sequence<Is...>)
    {
        (void)((index == Is ? (fn(std::integral_constant<size_t, Is>{}), true) : false) || ...);
    }

    template <size_t PrimaryIndex, size_t Index, typename StoresTuple, typename ComponentsVec>
    static decltype(auto) viewComponent(StoresTuple& stores, ComponentsVec& components, Entity entity, size_t denseIndex)
    {
        auto* store = std::get<Index>(stores);
        if constexpr (Index == PrimaryIndex)
        {
            return (components[denseIndex]);
        }
        else
        {
            return store->get(entity);
        }
    }

    template <size_t PrimaryIndex, typename Func, typename StoresTuple, typename ComponentsVec, size_t... Is>
    static void
    callView(Func& fn, StoresTuple& stores, ComponentsVec& components, Entity entity, size_t denseIndex, std::index_sequence<Is...>)
    {
        fn(entity, viewComponent<PrimaryIndex, Is>(stores, components, entity, denseIndex)...);
    }

    void ensureCompositionEntry(Entity entity)
    {
        if (entity.index >= m_entityComposition.size())
        {
            m_entityComposition.resize(entity.index + 1);
        }
    }

    bool ensureAlive(Entity entity, const char* action) const
    {
        if (!m_entityManager.isAlive(entity))
        {
            logDead(action, entity);
            return false;
        }
        return true;
    }

    void logDead(const char* action, Entity entity) const
    {
        std::fprintf(stderr,
                     "GameEngine: %s: dead entity idx=%u gen=%u ignored\n",
                     action,
                     static_cast<unsigned>(entity.index),
                     static_cast<unsigned>(entity.generation));
    }

    void logTypeNameMismatch(const std::string& existing, const std::string& incoming) const
    {
        std::fprintf(stderr,
                     "GameEngine: Component type registered with two names: existing='%s' new='%s'\n",
                     existing.c_str(),
                     incoming.c_str());
    }

    void logTypeNameCollision(const std::string& typeName) const
    {
        std::fprintf(stderr, "GameEngine: Component type name '%s' already mapped to a different type\n", typeName.c_str());
    }

    void logTypeLookupFailure(const std::string& typeName) const
    {
        std::fprintf(stderr, "GameEngine: Component type name '%s' not registered\n", typeName.c_str());
    }

    void trackComponentAdd(Entity entity, std::type_index typeIdx)
    {
        ensureCompositionEntry(entity);
        auto& types = m_entityComposition[entity.index];
        if (std::find(types.begin(), types.end(), typeIdx) == types.end())
        {
            types.push_back(typeIdx);
        }
    }

    void trackComponentRemove(Entity entity, std::type_index typeIdx)
    {
        if (entity.index >= m_entityComposition.size())
        {
            return;
        }

        auto& types = m_entityComposition[entity.index];
        auto  it    = std::find(types.begin(), types.end(), typeIdx);
        if (it != types.end())
        {
            *it = types.back();
            types.pop_back();
        }
    }

    bool compositionHas(Entity entity, std::type_index typeIdx) const
    {
        if (entity.index >= m_entityComposition.size())
        {
            return false;
        }

        const auto& types = m_entityComposition[entity.index];
        return std::find(types.begin(), types.end(), typeIdx) != types.end();
    }

    EntityManager       m_entityManager;  ///< Allocates/destroys entities with generations
    std::vector<Entity> m_entities;       ///< All active entities

    /// Per-type component stores, keyed by type_index
    std::unordered_map<std::type_index, std::unique_ptr<IComponentStore>> m_componentStores;

    /// Stable type name mapping for serialization
    std::unordered_map<std::type_index, std::string> m_typeNames;
    std::unordered_map<std::string, std::type_index> m_nameToType;

    /// Deferred structural changes (add/remove/etc.)
    std::vector<std::function<void()>> m_commandBuffer;

    /// Entities scheduled for deferred destruction
    std::vector<Entity> m_deferredDestroy;

    /// Per-entity component composition (type_index list)
    std::vector<std::vector<std::type_index>> m_entityComposition;
};

#endif  // REGISTRY_H
