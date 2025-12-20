#include "AudioManager.h"

#include <algorithm>

#include <ActionBinding.h>
#include <AudioTypes.h>
#include <Components.h>
#include <Logger.h>
#include <SAudio.h>
#include <SystemLocator.h>
#include <World.h>

namespace Example
{

void AudioManager::onCreate(Entity self, World& world)
{
    Systems::SAudio& audio = Systems::SystemLocator::audio();

    // Apply persisted (or default) master volume.
    audio.setMasterVolume(m_currentMasterVolume);
    LOG_INFO_CONSOLE("AudioManager: Master volume set to {}%", static_cast<int>(m_currentMasterVolume * 100.0f));

    // Load audio assets
    audio.loadSound("background_music", "assets/audio/rainyday.mp3", AudioType::Music);
    audio.loadSound("motor_boat", "assets/audio/motor_boat.mp3", AudioType::SFX);
    audio.loadSound("sway", "assets/audio/sway.mp3", AudioType::SFX);

    // Start background music
    audio.playMusic("background_music", true);
    audio.setMusicVolume(kMusicVolume);

    // Bind input actions for volume control
    Components::CInputController* input = world.components().tryGet<Components::CInputController>(self);
    if (!input)
    {
        input = world.components().add<Components::CInputController>(self);
    }

    {
        ActionBinding up;
        up.keys        = {KeyCode::Up};
        up.trigger     = ActionTrigger::Pressed;
        up.allowRepeat = true;
        input->bindings["VolumeUp"].push_back({up, 0});
    }
    {
        ActionBinding down;
        down.keys        = {KeyCode::Down};
        down.trigger     = ActionTrigger::Pressed;
        down.allowRepeat = true;
        input->bindings["VolumeDown"].push_back({down, 0});
    }

    LOG_INFO_CONSOLE("Audio initialized. Use Up/Down arrows to adjust volume.");
}

void AudioManager::onUpdate(float /*deltaTime*/, Entity self, World& world)
{
    Components::CInputController* input = world.components().tryGet<Components::CInputController>(self);
    if (!input)
    {
        return;
    }

    if (input->isActionActive("VolumeUp"))
    {
        adjustMasterVolume(+kVolumeStep);
    }
    if (input->isActionActive("VolumeDown"))
    {
        adjustMasterVolume(-kVolumeStep);
    }
}

void AudioManager::adjustMasterVolume(float delta)
{
    m_currentMasterVolume = std::clamp(m_currentMasterVolume + delta, 0.0f, 1.0f);
    Systems::SystemLocator::audio().setMasterVolume(m_currentMasterVolume);
    LOG_INFO_CONSOLE("Master volume: {}%", static_cast<int>(m_currentMasterVolume * 100.0f));
}

void AudioManager::serializeFields(Serialization::ScriptFieldWriter& out) const
{
    out.setFloat("masterVolume", static_cast<double>(m_currentMasterVolume));
}

void AudioManager::deserializeFields(const Serialization::ScriptFieldReader& in)
{
    if (auto v = in.getFloat("masterVolume"))
    {
        m_currentMasterVolume = std::clamp(static_cast<float>(*v), 0.0f, 1.0f);
    }
}

}  // namespace Example
