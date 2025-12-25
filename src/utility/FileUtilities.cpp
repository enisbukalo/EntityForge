#include "FileUtilities.h"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace Internal
{

namespace
{
[[nodiscard]] std::string errnoString()
{
    const int err = errno;
    if (err == 0)
    {
        return std::string{};
    }
    const char* msg = std::strerror(err);
    return msg ? std::string(msg) : std::string{};
}

[[nodiscard]] std::string openErrorMessage(const std::filesystem::path& path)
{
    std::string       msg = "Could not open file: " + path.string();
    const std::string err = errnoString();
    if (!err.empty())
    {
        msg += " (" + err + ")";
    }
    return msg;
}

struct FileCloser
{
    void operator()(std::FILE* f) const noexcept
    {
        if (f)
        {
            std::fclose(f);
        }
    }
};

// Cross-platform fopen with Windows wide-path support.
[[nodiscard]] std::FILE* openFile(const std::filesystem::path& path, bool binary, bool write)
{
#ifdef _WIN32
    const wchar_t* mode = write ? (binary ? L"wb" : L"w") : (binary ? L"rb" : L"r");
    return _wfopen(path.c_str(), mode);
#else
    const char* mode = write ? (binary ? "wb" : "w") : (binary ? "rb" : "r");
    return std::fopen(path.string().c_str(), mode);
#endif
}

[[nodiscard]] std::vector<std::uint8_t> readAllBytes(const std::filesystem::path& path, bool binary)
{
    errno = 0;
    std::unique_ptr<std::FILE, FileCloser> file(openFile(path, binary, /*write=*/false));
    if (!file)
    {
        throw std::runtime_error(openErrorMessage(path));
    }

    // Safety limit: avoid attempting to allocate absurdly large buffers.
    constexpr size_t kMaxBytes = 256ull * 1024ull * 1024ull;

    std::vector<std::uint8_t> buffer;
    buffer.reserve(16 * 1024);

    std::uint8_t tmp[64 * 1024];
    size_t       total = 0;
    while (true)
    {
        const size_t n = std::fread(tmp, 1, sizeof(tmp), file.get());
        if (n > 0)
        {
            total += n;
            if (total > kMaxBytes)
            {
                throw std::runtime_error("File too large (exceeds 256 MiB): " + path.string());
            }
            buffer.insert(buffer.end(), tmp, tmp + n);
        }

        if (n < sizeof(tmp))
        {
            if (std::feof(file.get()))
            {
                break;
            }
            if (std::ferror(file.get()))
            {
                throw std::runtime_error("Could not read file: " + path.string());
            }
        }
    }

    return buffer;
}
}  // namespace

std::string FileUtilities::readFile(const std::string& path)
{
    const auto bytes = readAllBytes(std::filesystem::path(path), /*binary=*/false);
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

std::vector<std::uint8_t> FileUtilities::readFileBinary(const std::filesystem::path& path)
{
    return readAllBytes(path, /*binary=*/true);
}

void FileUtilities::writeFile(const std::string& path, const std::string& content)
{
    const std::filesystem::path p(path);
    errno = 0;
    std::unique_ptr<std::FILE, FileCloser> file(openFile(p, /*binary=*/false, /*write=*/true));
    if (!file)
    {
        throw std::runtime_error("Could not open file for writing: " + p.string());
    }

    if (!content.empty())
    {
        const size_t written = std::fwrite(content.data(), 1, content.size(), file.get());
        if (written != content.size())
        {
            throw std::runtime_error("Could not write file: " + p.string());
        }
    }
}

}  // namespace Internal
