#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <objectives/ObjectiveDefinition.h>

namespace Objectives
{

/**
 * @brief Loads and validates authored objective definitions.
 *
 * MVP contract:
 * - Loads *.json files in a directory (non-recursive)
 * - Validates unique IDs
 * - Validates prerequisites exist
 * - Validates dependency graph is acyclic
 */
class ObjectiveRegistry
{
public:
    ObjectiveRegistry()  = default;
    ~ObjectiveRegistry() = default;

    ObjectiveRegistry(const ObjectiveRegistry&)            = delete;
    ObjectiveRegistry(ObjectiveRegistry&&)                 = delete;
    ObjectiveRegistry& operator=(const ObjectiveRegistry&) = delete;
    ObjectiveRegistry& operator=(ObjectiveRegistry&&)      = delete;

    void clear();

    [[nodiscard]] bool loadFromDirectory(const std::filesystem::path& dir, std::vector<std::string>* outErrors = nullptr);

    [[nodiscard]] const ObjectiveDefinition* find(std::string_view id) const;
    [[nodiscard]] size_t                     size() const;

private:
    std::unordered_map<std::string, ObjectiveDefinition> m_definitions;
};

}  // namespace Objectives
