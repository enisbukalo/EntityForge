#include "ExecutablePaths.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

namespace Internal
{

std::filesystem::path ExecutablePaths::getExecutableDir()
{
#if defined(_WIN32)
    char        buffer[MAX_PATH] = {0};
    const DWORD len              = GetModuleFileNameA(nullptr, buffer, static_cast<DWORD>(sizeof(buffer)));
    if (len == 0 || len >= sizeof(buffer))
    {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(std::string(buffer, len)).parent_path();
#else
    char          buffer[PATH_MAX] = {0};
    const ssize_t len              = ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0)
    {
        return std::filesystem::current_path();
    }
    buffer[len] = '\0';
    return std::filesystem::path(buffer).parent_path();
#endif
}

std::filesystem::path ExecutablePaths::resolveRelativeToExecutableDir(const std::filesystem::path& path)
{
    if (path.is_relative())
    {
        return getExecutableDir() / path;
    }
    return path;
}

}  // namespace Internal
