#include "SaveGame.h"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "ExecutablePaths.h"
#include "FileUtilities.h"
#include "Logger.h"
#include "World.h"

namespace
{

std::string toIso8601Utc(std::chrono::system_clock::time_point tp)
{
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);

    std::tm utcTm {};
#if defined(_WIN32)
    gmtime_s(&utcTm, &t);
#else
    gmtime_r(&t, &utcTm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&utcTm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::filesystem::path resolveSaveFilePath(const std::string& slotFilename)
{
    const std::filesystem::path saveDir = Internal::ExecutablePaths::resolveRelativeToExecutableDir("saved_games");
    return saveDir / slotFilename;
}

}  // namespace

namespace Systems
{

std::string SaveGame::normalizeSlotFilename(const std::string& slotName)
{
    if (slotName.size() >= 5 && slotName.substr(slotName.size() - 5) == ".json")
    {
        return slotName;
    }
    return slotName + ".json";
}

bool SaveGame::saveWorld(const World& world, const std::string& slotName)
{
    try
    {
        const std::string slotFilename = normalizeSlotFilename(slotName);
        const auto        filePath     = resolveSaveFilePath(slotFilename);

        {
            std::error_code ec;
            std::filesystem::create_directories(filePath.parent_path(), ec);
            if (ec)
            {
                LOG_ERROR("SaveGame: failed to create save directory '{}': {}", filePath.parent_path().string(), ec.message());
                return false;
            }
        }

        nlohmann::json root;
        root["format"]     = "GameEngineSave";
        root["version"]    = 1;
        root["engine"]     = { {"name", "GameEngine"}, {"semver", "0.1.0"} };
        root["createdUtc"] = toIso8601Utc(std::chrono::system_clock::now());
        root["entities"]   = nlohmann::json::array();

        const auto& entities = world.getEntities();
        size_t      id       = 0;
        for (Entity e : entities)
        {
            if (!world.isAlive(e))
            {
                continue;
            }

            nlohmann::json entityJson;
            entityJson["id"]         = std::to_string(id++);
            entityJson["components"] = nlohmann::json::array();
            root["entities"].push_back(std::move(entityJson));
        }

        Internal::FileUtilities::writeFile(filePath.string(), root.dump(2));
        LOG_INFO("SaveGame: saved '{}' to {}", slotName, filePath.string());
        return true;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("SaveGame: failed to save '{}': {}", slotName, ex.what());
        return false;
    }
}

bool SaveGame::loadWorld(World& world, const std::string& slotName, LoadMode mode)
{
    try
    {
        const std::string slotFilename = normalizeSlotFilename(slotName);
        const auto        filePath     = resolveSaveFilePath(slotFilename);

        const std::string content = Internal::FileUtilities::readFile(filePath.string());
        const auto        root    = nlohmann::json::parse(content);

        if (!root.is_object() || root.value("format", "") != "GameEngineSave")
        {
            LOG_ERROR("SaveGame: invalid save format in {}", filePath.string());
            return false;
        }

        const int version = root.value("version", 0);
        if (version != 1)
        {
            LOG_ERROR("SaveGame: unsupported save version {} in {}", version, filePath.string());
            return false;
        }

        if (mode == LoadMode::ReplaceWorld)
        {
            world.clear();
        }

        if (root.contains("entities") && root["entities"].is_array())
        {
            for (const auto& entityJson : root["entities"])
            {
                (void)entityJson;
                world.createEntity();
            }
        }

        LOG_INFO("SaveGame: loaded '{}' from {}", slotName, filePath.string());
        return true;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("SaveGame: failed to load '{}': {}", slotName, ex.what());
        return false;
    }
}

}  // namespace Systems
