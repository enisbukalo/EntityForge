#pragma once

#include <cstddef>
#include <random>

#include <Components.h>
#include <Vec2.h>

namespace Example
{

class BarrelSpawner final : public Components::INativeScript
{
public:
    BarrelSpawner(float minX, float maxX, float minY, float maxY, size_t count);

    void onCreate(Entity self, World& world) override;
    void onUpdate(float deltaTime, Entity self, World& world) override;

private:
    float  m_minX;
    float  m_maxX;
    float  m_minY;
    float  m_maxY;
    size_t m_barrelCount;

    std::mt19937                          m_rng;
    std::uniform_real_distribution<float> m_distX;
    std::uniform_real_distribution<float> m_distY;
};

}  // namespace Example
