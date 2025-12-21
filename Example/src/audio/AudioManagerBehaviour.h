#pragma once

#include <Components.h>
#include <ISerializableScript.h>

namespace Example
{

class AudioManagerBehaviour final : public Components::INativeScript, public Components::ISerializableScript
{
public:
    static constexpr const char* kScriptName = "AudioManager";

    void onCreate(Entity self, World& world) override;
    void onUpdate(float deltaTime, Entity self, World& world) override;

    void serializeFields(Serialization::ScriptFieldWriter& out) const override;
    void deserializeFields(const Serialization::ScriptFieldReader& in) override;

private:
    void adjustMasterVolume(float delta);

    static constexpr float kInitialMasterVolume = 0.15f;
    static constexpr float kMusicVolume         = 0.80f;
    static constexpr float kVolumeStep          = 0.05f;

    float m_currentMasterVolume = kInitialMasterVolume;
};

}  // namespace Example
