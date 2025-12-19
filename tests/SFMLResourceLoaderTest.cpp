#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Image.hpp>

#include "SFMLResourceLoader.h"

namespace
{

static std::filesystem::path makeTempPath(const std::string& filename)
{
    const auto tempDir = std::filesystem::temp_directory_path();
    return tempDir / filename;
}

static void writeBinaryFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open()) << "Failed to open temp file for writing: " << path.string();
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    ASSERT_TRUE(out.good()) << "Failed to write bytes to: " << path.string();
}

static std::vector<std::uint8_t> oneByOnePngBytes()
{
    // Valid 1x1 RGBA PNG.
    return {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,
        0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
        0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
        0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41,
        0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,
        0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,
        0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,
        0x42, 0x60, 0x82};
}

static std::vector<std::uint8_t> tinyWavBytes()
{
    // Minimal PCM WAV (8-bit, mono, 8000 Hz) with 1 sample.
    return {
        'R',  'I',  'F',  'F',  0x25, 0x00, 0x00, 0x00,  // RIFF size = 37
        'W',  'A',  'V',  'E',
        'f',  'm',  't',  ' ',  0x10, 0x00, 0x00, 0x00,  // fmt chunk size = 16
        0x01, 0x00,                                            // PCM
        0x01, 0x00,                                            // channels = 1
        0x40, 0x1F, 0x00, 0x00,                                // sampleRate = 8000
        0x40, 0x1F, 0x00, 0x00,                                // byteRate = 8000
        0x01, 0x00,                                            // blockAlign = 1
        0x08, 0x00,                                            // bitsPerSample = 8
        'd',  'a',  't',  'a',  0x01, 0x00, 0x00, 0x00,  // data chunk size = 1
        0x80                                                  // one sample (silence-ish)
    };
}

}  // namespace

TEST(SFMLResourceLoaderTest, ImageLoadFailsForMissingFile)
{
    sf::Image    image;
    std::string  err;
    const auto   path   = makeTempPath("sfml_loader_missing_texture.png");
    const bool   loaded = Internal::SFMLResourceLoader::loadImageFromFileBytes(path, image, &err);
    EXPECT_FALSE(loaded);
    EXPECT_FALSE(err.empty());
}

TEST(SFMLResourceLoaderTest, ImageLoadSucceedsForValidPngBytes)
{
    const auto path = makeTempPath("sfml_loader_1x1.png");
    writeBinaryFile(path, oneByOnePngBytes());

    sf::Image   image;
    std::string err;
    const bool  loaded = Internal::SFMLResourceLoader::loadImageFromFileBytes(path, image, &err);

    EXPECT_TRUE(loaded) << err;
    EXPECT_EQ(image.getSize().x, 1u);
    EXPECT_EQ(image.getSize().y, 1u);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(SFMLResourceLoaderTest, ImageLoadFailsForInvalidBytes)
{
    const auto path = makeTempPath("sfml_loader_invalid.bin");
    writeBinaryFile(path, {0x00, 0x01, 0x02, 0x03, 0x04});

    sf::Image   image;
    std::string err;
    const bool  loaded = Internal::SFMLResourceLoader::loadImageFromFileBytes(path, image, &err);

    EXPECT_FALSE(loaded);
    EXPECT_FALSE(err.empty());

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(SFMLResourceLoaderTest, SoundBufferLoadSucceedsForValidWavBytes)
{
    const auto path = makeTempPath("sfml_loader_tiny.wav");
    writeBinaryFile(path, tinyWavBytes());

    sf::SoundBuffer buffer;
    std::string     err;
    const bool      loaded = Internal::SFMLResourceLoader::loadSoundBufferFromFileBytes(path, buffer, &err);

    EXPECT_TRUE(loaded) << err;
    EXPECT_EQ(buffer.getChannelCount(), 1u);
    EXPECT_EQ(buffer.getSampleRate(), 8000u);
    EXPECT_EQ(buffer.getSampleCount(), 1u);

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

TEST(SFMLResourceLoaderTest, SoundBufferLoadFailsForInvalidBytes)
{
    const auto path = makeTempPath("sfml_loader_invalid.wav");
    writeBinaryFile(path, {0x10, 0x20, 0x30, 0x40});

    sf::SoundBuffer buffer;
    std::string     err;
    const bool      loaded = Internal::SFMLResourceLoader::loadSoundBufferFromFileBytes(path, buffer, &err);

    EXPECT_FALSE(loaded);
    EXPECT_FALSE(err.empty());

    std::error_code ec;
    std::filesystem::remove(path, ec);
}
