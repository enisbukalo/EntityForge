#include "SFMLResourceLoader.h"

#include <vector>

#include "FileUtilities.h"

namespace Internal
{

void SFMLResourceLoader::setError(std::string* outError, const std::string& message)
{
    if (outError)
    {
        *outError = message;
    }
}

bool SFMLResourceLoader::loadImageFromFileBytes(const std::filesystem::path& path, sf::Image& outImage, std::string* outError)
{
    std::vector<std::uint8_t> bytes;
    try
    {
        bytes = FileUtilities::readFileBinary(path);
    }
    catch (const std::exception& e)
    {
        setError(outError, std::string("exception reading file bytes: ") + e.what());
        return false;
    }

    if (bytes.empty())
    {
        setError(outError, "file was empty");
        return false;
    }

    bool      imageLoaded = false;
    try
    {
        imageLoaded = outImage.loadFromMemory(bytes.data(), bytes.size());
    }
    catch (const std::exception& e)
    {
        setError(outError, std::string("exception decoding image bytes: ") + e.what());
        return false;
    }

    if (!imageLoaded)
    {
        setError(outError, "sf::Image::loadFromMemory returned false");
        return false;
    }

    return true;
}

bool SFMLResourceLoader::loadTextureFromFileBytes(const std::filesystem::path& path, sf::Texture& outTexture, std::string* outError)
{
    sf::Image    image;
    std::string  imageError;

    if (!loadImageFromFileBytes(path, image, &imageError))
    {
        setError(outError, imageError);
        return false;
    }

    bool textureLoaded = false;
    try
    {
        textureLoaded = outTexture.loadFromImage(image);
    }
    catch (const std::exception& e)
    {
        setError(outError, std::string("exception uploading texture: ") + e.what());
        return false;
    }

    if (!textureLoaded)
    {
        setError(outError, "sf::Texture::loadFromImage returned false");
        return false;
    }

    return true;
}

bool SFMLResourceLoader::loadSoundBufferFromFileBytes(const std::filesystem::path& path,
                                                      sf::SoundBuffer&            outBuffer,
                                                      std::string*                outError)
{
    std::vector<std::uint8_t> bytes;
    try
    {
        bytes = FileUtilities::readFileBinary(path);
    }
    catch (const std::exception& e)
    {
        setError(outError, std::string("exception reading file bytes: ") + e.what());
        return false;
    }

    if (bytes.empty())
    {
        setError(outError, "file was empty");
        return false;
    }

    bool loaded = false;
    try
    {
        loaded = outBuffer.loadFromMemory(bytes.data(), bytes.size());
    }
    catch (const std::exception& e)
    {
        setError(outError, std::string("exception decoding sound bytes: ") + e.what());
        return false;
    }

    if (!loaded)
    {
        setError(outError, "sf::SoundBuffer::loadFromMemory returned false");
        return false;
    }

    return true;
}

}  // namespace Internal
