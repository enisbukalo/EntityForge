#ifndef COMPONENT_SERIALIZATION_REGISTRY_H
#define COMPONENT_SERIALIZATION_REGISTRY_H

#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

#include "Entity.h"

class World;

namespace Serialization
{

struct SaveContext
{
    const std::unordered_map<Entity, std::string>* entityToSavedId = nullptr;
};

struct LoadContext
{
    const std::unordered_map<std::string, Entity>* savedIdToEntity = nullptr;
};

class ComponentSerializationRegistry
{
public:
    using HasFn        = std::function<bool(const World&, Entity)>;
    using SerializeFn  = std::function<nlohmann::json(const World&, Entity, const SaveContext&)>;
    using DeserializeFn = std::function<void(World&, Entity, const nlohmann::json&, const LoadContext&)>;

    struct Entry
    {
        std::string  stableName;
        HasFn        has;
        SerializeFn  serialize;
        DeserializeFn deserialize;
    };

    ComponentSerializationRegistry()  = default;
    ~ComponentSerializationRegistry() = default;

    ComponentSerializationRegistry(const ComponentSerializationRegistry&)            = delete;
    ComponentSerializationRegistry& operator=(const ComponentSerializationRegistry&) = delete;

    void registerComponent(const std::string& stableName, HasFn has, SerializeFn serialize, DeserializeFn deserialize)
    {
        if (stableName.empty())
        {
            return;
        }

        auto it = m_indexByName.find(stableName);
        if (it != m_indexByName.end())
        {
            // Keep first registration; this avoids accidental double-registration.
            return;
        }

        Entry entry;
        entry.stableName = stableName;
        entry.has        = std::move(has);
        entry.serialize  = std::move(serialize);
        entry.deserialize = std::move(deserialize);

        m_indexByName.emplace(entry.stableName, m_entries.size());
        m_entries.push_back(std::move(entry));
    }

    bool has(std::string_view stableName) const
    {
        return m_indexByName.find(std::string(stableName)) != m_indexByName.end();
    }

    const Entry* tryGet(std::string_view stableName) const
    {
        auto it = m_indexByName.find(std::string(stableName));
        if (it == m_indexByName.end())
        {
            return nullptr;
        }
        return &m_entries[it->second];
    }

    const std::vector<Entry>& entries() const
    {
        return m_entries;
    }

private:
    std::vector<Entry>                       m_entries;
    std::unordered_map<std::string, size_t>  m_indexByName;
};

}  // namespace Serialization

#endif  // COMPONENT_SERIALIZATION_REGISTRY_H
