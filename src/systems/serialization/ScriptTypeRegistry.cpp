#include "ScriptTypeRegistry.h"

namespace Serialization
{

ScriptTypeRegistry& ScriptTypeRegistry::instance()
{
    static ScriptTypeRegistry registry;
    return registry;
}

bool ScriptTypeRegistry::has(const std::string& stableName) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_factories.find(stableName) != m_factories.end();
}

std::unique_ptr<Components::INativeScript> ScriptTypeRegistry::create(const std::string& stableName) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_factories.find(stableName);
    if (it == m_factories.end())
    {
        return nullptr;
    }
    return it->second();
}

std::string ScriptTypeRegistry::stableNameFor(const std::type_index& typeIdx) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_stableNameByType.find(typeIdx);
    if (it == m_stableNameByType.end())
    {
        return std::string {};
    }

    return it->second;
}

}  // namespace Serialization
