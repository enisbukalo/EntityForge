#ifndef EXECUTABLE_PATHS_H
#define EXECUTABLE_PATHS_H

#include <filesystem>

namespace Internal
{

/**
 * @brief Utility class for resolving paths relative to the running executable.
 */
class ExecutablePaths
{
public:
    /**
     * @brief Returns the directory that contains the running executable.
     *
     * If resolution fails, returns std::filesystem::current_path().
     */
    static std::filesystem::path getExecutableDir();

    /**
     * @brief Resolves a path relative to the executable directory.
     *
     * If @p path is absolute, it is returned unchanged.
     */
    static std::filesystem::path resolveRelativeToExecutableDir(const std::filesystem::path& path);

private:
    ExecutablePaths() = delete;
};

}  // namespace Internal

#endif  // EXECUTABLE_PATHS_H
