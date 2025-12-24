#include <ObjectiveRegistry.h>

#include <FileUtilities.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <unordered_map>

namespace Objectives
{

namespace
{
using json = nlohmann::json;

std::string toErrorPrefix(const std::filesystem::path& file)
{
    if (file.empty())
    {
        return "ObjectiveRegistry: ";
    }
    return "ObjectiveRegistry: " + file.string() + ": ";
}

bool parseDefinition(const json& j, const std::filesystem::path& sourceFile, ObjectiveDefinition& outDef, std::vector<std::string>& errors)
{
    if (!j.is_object())
    {
        errors.push_back(toErrorPrefix(sourceFile) + "definition must be a JSON object");
        return false;
    }

    ObjectiveDefinition def;
    def.sourceFile  = sourceFile;
    def.id          = j.value("id", std::string{});
    def.title       = j.value("title", std::string{});
    def.description = j.value("description", std::string{});
    def.onComplete  = j.value("onComplete", std::string{});

    // activationConditions
    if (j.contains("activationConditions"))
    {
        const auto& conds = j.at("activationConditions");
        if (!conds.is_array())
        {
            errors.push_back(toErrorPrefix(sourceFile) + "'activationConditions' must be an array");
        }
        else
        {
            for (const auto& cj : conds)
            {
                if (!cj.is_object())
                {
                    errors.push_back(toErrorPrefix(sourceFile) + "'activationConditions' entries must be objects");
                    continue;
                }

                const std::string type = cj.value("type", std::string{});
                if (type == "FlagSet")
                {
                    ActivationCondition c;
                    c.type = ActivationConditionType::FlagSet;
                    c.flag = cj.value("flag", std::string{});
                    if (c.flag.empty())
                    {
                        errors.push_back(toErrorPrefix(sourceFile) + "FlagSet condition requires non-empty 'flag'");
                        continue;
                    }
                    def.activationConditions.push_back(std::move(c));
                }
                else if (type == "CounterAtLeast")
                {
                    ActivationCondition c;
                    c.type  = ActivationConditionType::CounterAtLeast;
                    c.key   = cj.value("key", std::string{});
                    c.value = cj.value("value", static_cast<std::int64_t>(0));
                    if (c.key.empty())
                    {
                        errors.push_back(toErrorPrefix(sourceFile)
                                         + "CounterAtLeast condition requires non-empty 'key'");
                        continue;
                    }
                    def.activationConditions.push_back(std::move(c));
                }
                else
                {
                    errors.push_back(toErrorPrefix(sourceFile) + "unknown activation condition type='" + type + "'");
                }
            }
        }
    }

    // progression
    if (j.contains("progression"))
    {
        const auto& pj = j.at("progression");
        if (!pj.is_object())
        {
            errors.push_back(toErrorPrefix(sourceFile) + "'progression' must be an object");
        }
        else
        {
            const std::string mode = pj.value("mode", std::string{"targeted"});
            if (mode == "targeted")
            {
                def.progression.mode = ProgressionMode::Targeted;
            }
            else if (mode == "signals")
            {
                def.progression.mode = ProgressionMode::Signals;
                if (pj.contains("signals"))
                {
                    const auto& sj = pj.at("signals");
                    if (!sj.is_array())
                    {
                        errors.push_back(toErrorPrefix(sourceFile) + "'progression.signals' must be an array");
                    }
                    else
                    {
                        for (const auto& s : sj)
                        {
                            if (!s.is_string())
                            {
                                errors.push_back(toErrorPrefix(sourceFile)
                                                 + "'progression.signals' entries must be strings");
                                continue;
                            }
                            const std::string sid = s.get<std::string>();
                            if (!sid.empty())
                            {
                                def.progression.signals.push_back(sid);
                            }
                        }
                    }
                }
            }
            else if (mode == "triggers")
            {
                def.progression.mode = ProgressionMode::Triggers;
                if (pj.contains("triggers"))
                {
                    const auto& tj = pj.at("triggers");
                    if (!tj.is_array())
                    {
                        errors.push_back(toErrorPrefix(sourceFile) + "'progression.triggers' must be an array");
                    }
                    else
                    {
                        for (const auto& t : tj)
                        {
                            if (!t.is_object())
                            {
                                errors.push_back(toErrorPrefix(sourceFile)
                                                 + "'progression.triggers' entries must be objects");
                                continue;
                            }

                            TriggerRule       rule;
                            const std::string et = t.value("type", std::string{"Enter"});
                            if (et == "Enter")
                            {
                                rule.type = TriggerEventType::Enter;
                            }
                            else if (et == "Exit")
                            {
                                rule.type = TriggerEventType::Exit;
                            }
                            else
                            {
                                errors.push_back(toErrorPrefix(sourceFile) + "unknown trigger type='" + et + "'");
                                continue;
                            }

                            rule.triggerName = t.value("triggerName", std::string{});
                            if (rule.triggerName.empty())
                            {
                                errors.push_back(toErrorPrefix(sourceFile)
                                                 + "trigger rule requires non-empty 'triggerName'");
                                continue;
                            }

                            const std::string action = t.value("action", std::string{"Complete"});
                            if (action == "Complete")
                            {
                                rule.action = TriggerAction::Complete;
                            }
                            else if (action == "IncrementCounter")
                            {
                                rule.action = TriggerAction::IncrementCounter;
                                rule.key    = t.value("key", std::string{});
                                rule.delta  = t.value("delta", static_cast<std::int64_t>(1));
                                if (rule.key.empty())
                                {
                                    errors.push_back(toErrorPrefix(sourceFile)
                                                     + "IncrementCounter trigger rule requires non-empty 'key'");
                                    continue;
                                }
                            }
                            else
                            {
                                errors.push_back(toErrorPrefix(sourceFile) + "unknown trigger action='" + action + "'");
                                continue;
                            }

                            def.progression.triggers.push_back(std::move(rule));
                        }
                    }
                }
            }
            else
            {
                errors.push_back(toErrorPrefix(sourceFile) + "unknown progression mode='" + mode + "'");
            }
        }
    }

    if (j.contains("prerequisites"))
    {
        const auto& p = j.at("prerequisites");
        if (!p.is_array())
        {
            errors.push_back(toErrorPrefix(sourceFile) + "'prerequisites' must be an array");
        }
        else
        {
            for (const auto& item : p)
            {
                if (!item.is_string())
                {
                    errors.push_back(toErrorPrefix(sourceFile) + "'prerequisites' entries must be strings");
                    continue;
                }
                def.prerequisites.push_back(item.get<std::string>());
            }
        }
    }

    if (def.id.empty())
    {
        errors.push_back(toErrorPrefix(sourceFile) + "missing required field 'id'");
    }
    if (def.title.empty())
    {
        errors.push_back(toErrorPrefix(sourceFile) + "missing required field 'title' for id='" + def.id + "'");
    }
    if (def.description.empty())
    {
        errors.push_back(toErrorPrefix(sourceFile) + "missing required field 'description' for id='" + def.id + "'");
    }

    outDef = std::move(def);
    return true;
}

bool detectCycles(const std::unordered_map<std::string, ObjectiveDefinition>& defs, std::vector<std::string>& errors)
{
    enum class Mark
    {
        Unvisited,
        Visiting,
        Visited
    };

    std::unordered_map<std::string, Mark> marks;
    marks.reserve(defs.size());
    for (const auto& [id, _] : defs)
    {
        marks.emplace(id, Mark::Unvisited);
    }

    std::function<bool(const std::string&)> dfs;
    dfs = [&](const std::string& id) -> bool
    {
        auto it = marks.find(id);
        if (it == marks.end())
        {
            return false;
        }

        if (it->second == Mark::Visiting)
        {
            errors.push_back("ObjectiveRegistry: cycle detected involving id='" + id + "'");
            return true;
        }
        if (it->second == Mark::Visited)
        {
            return false;
        }

        it->second = Mark::Visiting;

        const auto& defIt = defs.find(id);
        if (defIt != defs.end())
        {
            for (const auto& prereq : defIt->second.prerequisites)
            {
                // Missing prereqs are validated elsewhere.
                if (defs.find(prereq) == defs.end())
                {
                    continue;
                }
                if (dfs(prereq))
                {
                    return true;
                }
            }
        }

        it->second = Mark::Visited;
        return false;
    };

    for (const auto& [id, _] : defs)
    {
        if (dfs(id))
        {
            return true;
        }
    }

    return false;
}

}  // namespace

void ObjectiveRegistry::clear()
{
    m_definitions.clear();
}

bool ObjectiveRegistry::loadFromDirectory(const std::filesystem::path& dir, std::vector<std::string>* outErrors)
{
    clear();

    std::vector<std::string> errors;

    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec))
    {
        errors.push_back("ObjectiveRegistry: directory not found: " + dir.string());
        if (outErrors)
        {
            outErrors->insert(outErrors->end(), errors.begin(), errors.end());
        }
        return false;
    }

    std::unordered_map<std::string, std::filesystem::path> firstDefinitionFileById;

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (ec)
        {
            break;
        }

        if (!entry.is_regular_file(ec))
        {
            continue;
        }

        const auto& path = entry.path();
        if (path.extension() != ".json")
        {
            continue;
        }

        try
        {
            const std::string text = Internal::FileUtilities::readFile(path.string());
            json              root = json::parse(text);

            auto loadOne = [&](const json& obj)
            {
                ObjectiveDefinition def;
                (void)parseDefinition(obj, path, def, errors);
                if (def.id.empty())
                {
                    return;
                }

                auto firstIt = firstDefinitionFileById.find(def.id);
                if (firstIt != firstDefinitionFileById.end())
                {
                    errors.push_back("ObjectiveRegistry: duplicate objective id='" + def.id + "' in '" + path.string()
                                     + "' (already defined in '" + firstIt->second.string() + "')");
                    return;
                }

                firstDefinitionFileById.emplace(def.id, path);
                m_definitions.emplace(def.id, std::move(def));
            };

            if (root.is_object())
            {
                loadOne(root);
            }
            else if (root.is_array())
            {
                for (const auto& obj : root)
                {
                    loadOne(obj);
                }
            }
            else
            {
                errors.push_back(toErrorPrefix(path) + "root must be an object or array");
            }
        }
        catch (const std::exception& e)
        {
            errors.push_back(toErrorPrefix(path) + std::string("failed to parse JSON: ") + e.what());
        }
    }

    // Validate prerequisites exist.
    for (const auto& [id, def] : m_definitions)
    {
        for (const auto& prereq : def.prerequisites)
        {
            if (m_definitions.find(prereq) == m_definitions.end())
            {
                errors.push_back("ObjectiveRegistry: objective id='" + id + "' references missing prerequisite id='"
                                 + prereq + "'");
            }
        }
    }

    // Validate graph acyclic.
    (void)detectCycles(m_definitions, errors);

    if (outErrors)
    {
        outErrors->insert(outErrors->end(), errors.begin(), errors.end());
    }

    return errors.empty();
}

const ObjectiveDefinition* ObjectiveRegistry::find(std::string_view id) const
{
    auto it = m_definitions.find(std::string(id));
    if (it == m_definitions.end())
    {
        return nullptr;
    }
    return &it->second;
}

size_t ObjectiveRegistry::size() const
{
    return m_definitions.size();
}

}  // namespace Objectives
