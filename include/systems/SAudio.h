#ifndef SAUDIO_H
#define SAUDIO_H

#include <SFML/Audio.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "IAudioSystem.h"
#include "System.h"

class World;

namespace Systems
{

/**
 * @brief SFML-based implementation of the audio system
 *
 * @description
 * SAudio implements audio playback using SFML's audio module.
 * It manages a fixed-size pool of sf::Sound objects for SFX playback
 * and a single sf::Music object for streamed music playback.
 *
 * Features:
 * - SFX pooling: Reuses sf::Sound objects for efficient playback
 * - Music streaming: Single active music track with streaming
 * - Volume control: Master volume plus per-source volume; simple music volume
 *
 * Thread Safety:
 * All methods should be called from the main thread. SFML audio operations
 * are not guaranteed to be thread-safe.
 */
class SAudio : public IAudioSystem, public System
{
public:
    /**
     * @brief Construct audio system with specified pool size
     * @param poolSize Number of simultaneous sound effects (default 32)
     */
    explicit SAudio(size_t poolSize = AudioConstants::DEFAULT_SFX_POOL_SIZE);

    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~SAudio() override;

    // IAudioSystem interface implementation
    bool initialize() override;
    void shutdown() override;

    bool loadSound(const std::string& id, const std::string& filepath, AudioType type) override;
    void unloadSound(const std::string& id) override;

    bool playSfx(Entity entity, const std::string& id, bool loop = false, float volume = 1.0f) override;
    void stopSfx(Entity entity) override;
    void setSfxVolume(Entity entity, float volume) override;

    bool playMusic(const std::string& id, bool loop = true) override;
    void stopMusic() override;
    void pauseMusic() override;
    void resumeMusic() override;
    bool isMusicPlaying() const override;

    void setMasterVolume(float volume) override;
    void setMusicVolume(float volume) override;

    float getMasterVolume() const override;
    float getMusicVolume() const override;

    void update(float deltaTime) override;

    // ISystem interface implementation
    void        update(float deltaTime, World& world) override;
    UpdateStage stage() const override
    {
        return UpdateStage::PostFlush;
    }

    std::string_view name() const override
    {
        return "SAudio";
    }

    /**
     * @brief ECS-driven audio update that consumes component data
     */
    void updateEcs(float deltaTime, World& world);

    // Delete copy and move constructors/assignment operators
    SAudio(const SAudio&)            = delete;
    SAudio(SAudio&&)                 = delete;
    SAudio& operator=(const SAudio&) = delete;
    SAudio& operator=(SAudio&&)      = delete;

private:
    struct SoundSlot
    {
        sf::Sound sound;
        Entity    owner      = Entity::null();
        bool      inUse      = false;
        float     baseVolume = 1.0f;
    };

    /**
     * @brief Find an available slot in the sound pool
     * @return Index of available slot, or -1 if pool is full
     */
    int findAvailableSlot();

    float calculateEffectiveSfxVolume(float baseVolume) const;
    float calculateEffectiveMusicVolume(float baseVolume) const;

    void shutdownInternal();

    bool                                             m_initialized = false;
    std::vector<SoundSlot>                           m_soundPool;
    std::unordered_map<std::string, sf::SoundBuffer> m_soundBuffers;
    std::unordered_map<std::string, std::string>     m_musicPaths;  ///< Map music IDs to file paths
    std::unique_ptr<sf::Music>                       m_currentMusic;
    std::string                                      m_currentMusicId;
    float                                            m_currentMusicBaseVolume = 1.0f;

    float                              m_masterVolume = AudioConstants::DEFAULT_MASTER_VOLUME;
    float                              m_musicVolume  = AudioConstants::DEFAULT_MUSIC_VOLUME;
    float                              m_sfxVolume    = AudioConstants::DEFAULT_SFX_VOLUME;
    std::unordered_map<Entity, size_t> m_entityToSlot;
};

}  // namespace Systems

#endif  // SAUDIO_H
