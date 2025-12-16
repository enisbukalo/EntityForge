#ifndef SCRIPT_TYPE_REGISTRY_H
#define SCRIPT_TYPE_REGISTRY_H

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>

#include <CNativeScript.h>

namespace Serialization
{

/**
 * @brief Runtime registry for native script types.
 *
 * Maps a stable script type name (saved in JSON) to a factory function that can
 * construct a new script instance on load.
 */
class ScriptTypeRegistry
{
public:
    using FactoryFn = std::function<std::unique_ptr<Components::INativeScript>()>;

    static ScriptTypeRegistry& instance();

    ScriptTypeRegistry(const ScriptTypeRegistry&)            = delete;
    ScriptTypeRegistry& operator=(const ScriptTypeRegistry&) = delete;

    template <typename T>
    bool registerScript(const std::string& stableName)
    {
        static_assert(std::is_base_of<Components::INativeScript, T>::value,
                      "ScriptTypeRegistry::registerScript requires T to derive from Components::INativeScript");

        std::lock_guard<std::mutex> lock(m_mutex);

        const std::type_index typeIdx(typeid(T));

        auto existingFactoryIt = m_factories.find(stableName);
        if (existingFactoryIt != m_factories.end())
        {
            // Idempotent registrations are allowed as long as the stable name maps
            // to the same concrete C++ type.
            auto existingTypeIt = m_typeByStableName.find(stableName);
            if (existingTypeIt != m_typeByStableName.end() && existingTypeIt->second != typeIdx)
            {
                return false;
            }

            m_typeByStableName.insert_or_assign(stableName, typeIdx);
            m_stableNameByType.insert_or_assign(typeIdx, stableName);
            return true;
        }

        m_factories.emplace(stableName, []() { return std::make_unique<T>(); });
        m_typeByStableName.emplace(stableName, typeIdx);
        m_stableNameByType.insert_or_assign(typeIdx, stableName);
        return true;
    }

    [[nodiscard]] bool has(const std::string& stableName) const;

    [[nodiscard]] std::unique_ptr<Components::INativeScript> create(const std::string& stableName) const;

    [[nodiscard]] std::string stableNameFor(const std::type_index& typeIdx) const;

    template <typename T>
    [[nodiscard]] std::string stableNameFor() const
    {
        static_assert(std::is_base_of<Components::INativeScript, T>::value,
                      "ScriptTypeRegistry::stableNameFor requires T to derive from Components::INativeScript");

        const std::type_index       typeIdx(typeid(T));
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_stableNameByType.find(typeIdx);
        if (it == m_stableNameByType.end())
        {
            return std::string{};
        }
        return it->second;
    }

private:
    ScriptTypeRegistry() = default;

    mutable std::mutex m_mutex;

    std::unordered_map<std::string, FactoryFn>       m_factories;
    std::unordered_map<std::string, std::type_index> m_typeByStableName;
    std::unordered_map<std::type_index, std::string> m_stableNameByType;
};

}  // namespace Serialization

#endif  // SCRIPT_TYPE_REGISTRY_H
