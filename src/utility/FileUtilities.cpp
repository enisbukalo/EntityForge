#include "FileUtilities.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Internal
{

std::string FileUtilities::readFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::uint8_t> FileUtilities::readFileBinary(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open file: " + path.string());
    }

    file.seekg(0, std::ios::end);
    const std::streamoff size = file.tellg();
    if (size < 0)
    {
        throw std::runtime_error("Could not determine file size: " + path.string());
    }

    // Safety limit: avoid attempting to allocate absurdly large buffers.
    constexpr std::streamoff kMaxBytes = 256ll * 1024ll * 1024ll;
    if (size > kMaxBytes)
    {
        throw std::runtime_error("File too large (" + std::to_string(static_cast<long long>(size)) + " bytes): " + path.string());
    }

    std::vector<std::uint8_t> buffer(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!buffer.empty())
    {
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        if (!file)
        {
            throw std::runtime_error("Could not read file: " + path.string());
        }
    }
    return buffer;
}

void FileUtilities::writeFile(const std::string& path, const std::string& content)
{
    std::ofstream file(path);
    if (!file.is_open())
    {
        throw std::runtime_error("Could not open file for writing: " + path);
    }
    file << content;
}

}  // namespace Internal
