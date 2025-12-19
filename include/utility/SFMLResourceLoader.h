#ifndef SFML_RESOURCE_LOADER_H
#define SFML_RESOURCE_LOADER_H

#include <cstdint>
#include <filesystem>
#include <string>

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace Internal
{

class SFMLResourceLoader
{
public:
    // Note: Callers are responsible for ensuring any required OpenGL context is active
    // before calling texture-loading functions.

    // Headless-safe (CPU-only) decode.
    static bool loadImageFromFileBytes(const std::filesystem::path& path, sf::Image& outImage, std::string* outError = nullptr);

    static bool loadTextureFromFileBytes(const std::filesystem::path& path, sf::Texture& outTexture, std::string* outError = nullptr);

    static bool
    loadSoundBufferFromFileBytes(const std::filesystem::path& path, sf::SoundBuffer& outBuffer, std::string* outError = nullptr);

private:
    static void setError(std::string* outError, const std::string& message);
};

}  // namespace Internal

#endif
