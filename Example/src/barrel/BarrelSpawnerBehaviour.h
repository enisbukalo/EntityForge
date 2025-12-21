#pragma once

#include <cstddef>
#include <random>

#include <Components.h>
#include <ISerializableScript.h>

namespace Example
{

class BarrelSpawnerBehaviour final : public Components::INativeScript, public Components::ISerializableScript
{
public:
    static constexpr const char* kScriptName = "BarrelSpawner";

    BarrelSpawnerBehaviour();
    BarrelSpawnerBehaviour(float minX, float maxX, float minY, float maxY, size_t count);

    void onCreate(Entity self, World& world) override;
    void onUpdate(float deltaTime, Entity self, World& world) override;

    void serializeFields(Serialization::ScriptFieldWriter& out) const override;
    void deserializeFields(const Serialization::ScriptFieldReader& in) override;

private:
    void rebuildDistributions();

    float  m_minX;
    float  m_maxX;
    float  m_minY;
    float  m_maxY;
    size_t m_barrelCount;

    bool m_hasSpawned = false;

    std::mt19937                          m_rng;
    std::uniform_real_distribution<float> m_distX;
    std::uniform_real_distribution<float> m_distY;
};

}  // namespace Example
