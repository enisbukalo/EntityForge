#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <cstdint>
#include <vector>
#include "Entity.h"
#include "utility/Logger.h"

/**
 * @brief Allocates and recycles entity handles with generation counters
 */
class EntityManager
{
public:
    using EntityIndex = uint32_t;
    using Generation  = uint32_t;

    EntityManager()  = default;
    ~EntityManager() = default;

    EntityManager(const EntityManager&)            = delete;
    EntityManager& operator=(const EntityManager&) = delete;

    /**
     * @brief Creates a new entity handle
     */
    Entity create()
    {
        if (m_generation.empty())
        {
            // Reserve index 0 as the null entity
            m_generation.push_back(0);
        }

        Entity result;
        if (!m_free.empty())
        {
            EntityIndex idx = m_free.back();
            m_free.pop_back();
            result = Entity{idx, m_generation[idx]};
        }
        else
        {
            m_generation.push_back(0);
            result = Entity{static_cast<EntityIndex>(m_generation.size() - 1), 0};
        }

        LOG_DEBUG("Entity created: E{}:G{}", result.index, result.generation);
        return result;
    }

    /**
     * @brief Destroys an entity; bumps generation to invalidate stale handles
     */
    void destroy(Entity e)
    {
        if (!isAlive(e))
            return;
        LOG_DEBUG("Entity destroyed: E{}:G{}", e.index, e.generation);
        ++m_generation[e.index];
        m_free.push_back(e.index);
    }

    /**
     * @brief Checks if an entity handle is alive
     */
    bool isAlive(Entity e) const
    {
        return e.index < m_generation.size() && e.index != 0 && m_generation[e.index] == e.generation;
    }

    /**
     * @brief Resets all state
     */
    void clear()
    {
        m_generation.clear();
        m_free.clear();
    }

private:
    std::vector<Generation>  m_generation;
    std::vector<EntityIndex> m_free;
};

#endif  // ENTITY_MANAGER_H
