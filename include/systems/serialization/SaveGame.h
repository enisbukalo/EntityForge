#ifndef SAVE_GAME_H
#define SAVE_GAME_H

#include <string>

class World;

namespace Systems
{

enum class LoadMode
{
    ReplaceWorld,
    AppendWorld
};

/**
 * @brief Minimal save/load entry point.
 */
class SaveGame
{
public:
    static bool saveWorld(const World& world, const std::string& slotName);
    static bool loadWorld(World& world, const std::string& slotName, LoadMode mode = LoadMode::ReplaceWorld);

private:
    static std::string normalizeSlotFilename(const std::string& slotName);
};

}  // namespace Systems

#endif  // SAVE_GAME_H
