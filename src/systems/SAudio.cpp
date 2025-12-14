#include "SAudio.h"

#include <algorithm>

#include "CAudioListener.h"
#include "CAudioSource.h"
#include "Logger.h"
#include "World.h"

#ifndef _WIN32
#include <fcntl.h>
#include <unistd.h>
#endif

namespace Systems
{

SAudio::SAudio(size_t poolSize) : m_soundPool(poolSize) {}

SAudio::~SAudio()
{
    shutdownInternal();
}

bool SAudio::initialize()
{
    if (m_initialized)
    {
        return true;
    }

#ifndef _WIN32
    int oldStderr = -1;
    int devNull   = open("/dev/null", O_WRONLY);
    if (devNull != -1)
    {
        oldStderr = dup(STDERR_FILENO);
        dup2(devNull, STDERR_FILENO);
        close(devNull);
    }
#endif

    for (auto& slot : m_soundPool)
    {
        slot.inUse      = false;
        slot.owner      = Entity::null();
        slot.baseVolume = 1.0f;
    }

#ifndef _WIN32
    if (oldStderr != -1)
    {
        dup2(oldStderr, STDERR_FILENO);
        close(oldStderr);
    }
#endif

    m_initialized = true;
    LOG_INFO("SAudio: Initialized with sound pool size {}", m_soundPool.size());
    return true;
}

void SAudio::shutdown()
{
    shutdownInternal();
}

void SAudio::shutdownInternal()
{
    if (!m_initialized)
    {
        return;
    }

    for (auto& slot : m_soundPool)
    {
        if (slot.inUse)
        {
            slot.sound.stop();
        }
        slot.inUse = false;
        slot.owner = Entity::null();
    }

    if (m_currentMusic)
    {
        m_currentMusic->stop();
        m_currentMusic.reset();
    }

    m_entityToSlot.clear();
    m_soundBuffers.clear();
    m_musicPaths.clear();
    m_currentMusicId.clear();
    m_initialized = false;
    LOG_INFO("SAudio: Shutdown complete");
}

bool SAudio::loadSound(const std::string& id, const std::string& filepath, AudioType type)
{
    if (!m_initialized)
    {
        LOG_ERROR("Cannot load sound: audio system not initialized");
        return false;
    }

    if (type == AudioType::SFX)
    {
        if (m_soundBuffers.find(id) != m_soundBuffers.end())
        {
            return true;
        }

#ifndef _WIN32
        int oldStderr = -1;
        int devNull   = open("/dev/null", O_WRONLY);
        if (devNull != -1)
        {
            oldStderr = dup(STDERR_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
        }
#endif

        sf::SoundBuffer buffer;
        bool            success = buffer.loadFromFile(filepath);

#ifndef _WIN32
        if (oldStderr != -1)
        {
            dup2(oldStderr, STDERR_FILENO);
            close(oldStderr);
        }
#endif

        if (!success)
        {
            LOG_ERROR("Failed to load sound buffer from file: {}", filepath);
            return false;
        }

        m_soundBuffers[id] = std::move(buffer);
        LOG_INFO("SAudio: Loaded sound '{}' from '{}'", id, filepath);
        return true;
    }

    m_musicPaths[id] = filepath;
    LOG_INFO("SAudio: Registered music '{}' from '{}'", id, filepath);
    return true;
}

void SAudio::unloadSound(const std::string& id)
{
    auto bufferIt = m_soundBuffers.find(id);
    if (bufferIt != m_soundBuffers.end())
    {
        for (auto& slot : m_soundPool)
        {
            if (slot.inUse && slot.sound.getBuffer() == &bufferIt->second)
            {
                slot.sound.stop();
                m_entityToSlot.erase(slot.owner);
                slot.inUse = false;
                slot.owner = Entity::null();
            }
        }
        m_soundBuffers.erase(bufferIt);
        return;
    }

    auto musicIt = m_musicPaths.find(id);
    if (musicIt != m_musicPaths.end())
    {
        if (m_currentMusicId == id)
        {
            stopMusic();
        }
        m_musicPaths.erase(musicIt);
    }
}

bool SAudio::playSfx(Entity entity, const std::string& id, bool loop, float volume)
{
    if (!m_initialized || !entity.isValid())
    {
        return false;
    }

    auto bufferIt = m_soundBuffers.find(id);
    if (bufferIt == m_soundBuffers.end())
    {
        LOG_WARN("Sound buffer '{}' not found", id);
        return false;
    }

    stopSfx(entity);

    int slotIndex = findAvailableSlot();
    if (slotIndex < 0)
    {
        LOG_WARN("Sound pool full, cannot play '{}'", id);
        return false;
    }

    auto& slot = m_soundPool[slotIndex];
    slot.sound.setBuffer(bufferIt->second);
    slot.baseVolume = std::clamp(volume, AudioConstants::MIN_VOLUME, AudioConstants::MAX_VOLUME);
    slot.sound.setVolume(calculateEffectiveSfxVolume(slot.baseVolume) * 100.0f);
    slot.sound.setLoop(loop);
    slot.sound.play();
    slot.inUse = true;
    slot.owner = entity;

    m_entityToSlot[entity] = static_cast<size_t>(slotIndex);
    return true;
}

void SAudio::stopSfx(Entity entity)
{
    auto it = m_entityToSlot.find(entity);
    if (it == m_entityToSlot.end())
    {
        return;
    }

    size_t index = it->second;
    if (index < m_soundPool.size())
    {
        auto& slot = m_soundPool[index];
        if (slot.inUse)
        {
            slot.sound.stop();
        }
        slot.inUse = false;
        slot.owner = Entity::null();
    }

    m_entityToSlot.erase(it);
}

void SAudio::setSfxVolume(Entity entity, float volume)
{
    auto it = m_entityToSlot.find(entity);
    if (it == m_entityToSlot.end())
    {
        return;
    }

    size_t index = it->second;
    if (index >= m_soundPool.size())
    {
        return;
    }

    auto& slot = m_soundPool[index];
    if (!slot.inUse)
    {
        return;
    }

    slot.baseVolume = std::clamp(volume, AudioConstants::MIN_VOLUME, AudioConstants::MAX_VOLUME);
    slot.sound.setVolume(calculateEffectiveSfxVolume(slot.baseVolume) * 100.0f);
}

bool SAudio::playMusic(const std::string& id, bool loop)
{
    if (!m_initialized)
    {
        LOG_ERROR("Cannot play music: audio system not initialized");
        return false;
    }

    auto it = m_musicPaths.find(id);
    if (it == m_musicPaths.end())
    {
        LOG_ERROR("Music '{}' not found", id);
        return false;
    }

    if (m_currentMusic)
    {
        m_currentMusic->stop();
    }

#ifndef _WIN32
    int oldStderr = -1;
    int devNull   = open("/dev/null", O_WRONLY);
    if (devNull != -1)
    {
        oldStderr = dup(STDERR_FILENO);
        dup2(devNull, STDERR_FILENO);
        close(devNull);
    }
#endif

    m_currentMusic = std::make_unique<sf::Music>();
    bool success   = m_currentMusic->openFromFile(it->second);

#ifndef _WIN32
    if (oldStderr != -1)
    {
        dup2(oldStderr, STDERR_FILENO);
        close(oldStderr);
    }
#endif

    if (!success)
    {
        LOG_ERROR("Failed to open music file: {}", it->second);
        m_currentMusic.reset();
        return false;
    }

    m_currentMusicBaseVolume = 1.0f;
    m_currentMusic->setLoop(loop);
    m_currentMusic->setVolume(calculateEffectiveMusicVolume(m_currentMusicBaseVolume) * 100.0f);
    m_currentMusic->play();
    m_currentMusicId = id;
    return true;
}

void SAudio::stopMusic()
{
    if (m_currentMusic)
    {
        m_currentMusic->stop();
        m_currentMusic.reset();
        m_currentMusicId.clear();
    }
}

void SAudio::pauseMusic()
{
    if (m_currentMusic && m_currentMusic->getStatus() == sf::Music::Playing)
    {
        m_currentMusic->pause();
    }
}

void SAudio::resumeMusic()
{
    if (m_currentMusic && m_currentMusic->getStatus() == sf::Music::Paused)
    {
        m_currentMusic->play();
    }
}

bool SAudio::isMusicPlaying() const
{
    return m_currentMusic && m_currentMusic->getStatus() == sf::Music::Playing;
}

void SAudio::setMasterVolume(float volume)
{
    m_masterVolume = std::clamp(volume, AudioConstants::MIN_VOLUME, AudioConstants::MAX_VOLUME);

    for (auto& slot : m_soundPool)
    {
        if (slot.inUse)
        {
            slot.sound.setVolume(calculateEffectiveSfxVolume(slot.baseVolume) * 100.0f);
        }
    }

    if (m_currentMusic)
    {
        m_currentMusic->setVolume(calculateEffectiveMusicVolume(m_currentMusicBaseVolume) * 100.0f);
    }
}

void SAudio::setMusicVolume(float volume)
{
    m_musicVolume = std::clamp(volume, AudioConstants::MIN_VOLUME, AudioConstants::MAX_VOLUME);

    if (m_currentMusic)
    {
        m_currentMusic->setVolume(calculateEffectiveMusicVolume(m_currentMusicBaseVolume) * 100.0f);
    }
}

float SAudio::getMasterVolume() const
{
    return m_masterVolume;
}

float SAudio::getMusicVolume() const
{
    return m_musicVolume;
}

void SAudio::update(float deltaTime)
{
    (void)deltaTime;

    if (!m_initialized)
    {
        return;
    }

    for (auto& slot : m_soundPool)
    {
        if (!slot.inUse)
        {
            continue;
        }

        if (slot.sound.getStatus() == sf::Sound::Stopped)
        {
            m_entityToSlot.erase(slot.owner);
            slot.inUse = false;
            slot.owner = Entity::null();
        }
    }
}

void SAudio::update(float deltaTime, World& world)
{
    updateEcs(deltaTime, world);
}

void SAudio::updateEcs(float deltaTime, World& world)
{
    update(deltaTime);

    if (!m_initialized)
    {
        return;
    }

    bool listenerApplied = false;
    world.components().each<Components::CAudioListener>(
        [this, &listenerApplied](Entity, const Components::CAudioListener& listener)
        {
            if (listenerApplied)
            {
                return;
            }
            setMasterVolume(listener.masterVolume);
            setMusicVolume(listener.musicVolume);
            listenerApplied = true;
        });

    world.components().each<Components::CAudioSource>(
        [this](Entity entity, Components::CAudioSource& source)
        {
            if (source.stopRequested)
            {
                stopSfx(entity);
            }

            if (source.playRequested)
            {
                playSfx(entity, source.clipId, source.loop, source.volume);
            }

            source.playRequested = false;
            source.stopRequested = false;
        });
}

int SAudio::findAvailableSlot()
{
    for (size_t i = 0; i < m_soundPool.size(); ++i)
    {
        if (!m_soundPool[i].inUse)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

float SAudio::calculateEffectiveSfxVolume(float baseVolume) const
{
    return std::clamp(baseVolume * m_masterVolume, AudioConstants::MIN_VOLUME, AudioConstants::MAX_VOLUME);
}

float SAudio::calculateEffectiveMusicVolume(float baseVolume) const
{
    return std::clamp(baseVolume * m_musicVolume * m_masterVolume, AudioConstants::MIN_VOLUME, AudioConstants::MAX_VOLUME);
}

}  // namespace Systems
