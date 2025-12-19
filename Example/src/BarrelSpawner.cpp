#include "BarrelSpawner.h"

#include "Barrel.h"

#include <algorithm>
#include <cstdint>

#include <Logger.h>

#include <World.h>

namespace Example
{

BarrelSpawner::BarrelSpawner()
    : m_minX(0.0f),
      m_maxX(0.0f),
      m_minY(0.0f),
      m_maxY(0.0f),
      m_barrelCount(0),
      m_hasSpawned(false),
      m_rng(std::random_device{}()),
      m_distX(0.0f, 0.0f),
      m_distY(0.0f, 0.0f)
{
}

BarrelSpawner::BarrelSpawner(float minX, float maxX, float minY, float maxY, size_t count)
    : m_minX(minX),
      m_maxX(maxX),
      m_minY(minY),
      m_maxY(maxY),
      m_barrelCount(count),
      m_hasSpawned(false),
      m_rng(std::random_device{}()),
      m_distX(minX, maxX),
      m_distY(minY, maxY)
{
}

void BarrelSpawner::onCreate(Entity /*self*/, World& world)
{
    // If this spawner was loaded from a save, we don't want to duplicate already-saved barrels.
    if (m_hasSpawned)
    {
        return;
    }

    rebuildDistributions();

    LOG_INFO("BarrelSpawner: spawning {} barrels", static_cast<unsigned long long>(m_barrelCount));

    for (size_t i = 0; i < m_barrelCount; ++i)
    {
        if (i > 0 && (i % 25u) == 0u)
        {
            LOG_INFO("BarrelSpawner: spawned {}/{}", static_cast<unsigned long long>(i), static_cast<unsigned long long>(m_barrelCount));
        }
        const Vec2 pos{m_distX(m_rng), m_distY(m_rng)};
        (void)spawnBarrel(world, pos);
    }

    LOG_INFO("BarrelSpawner: finished spawning {} barrels", static_cast<unsigned long long>(m_barrelCount));

    m_hasSpawned = true;
}

void BarrelSpawner::onUpdate(float /*deltaTime*/, Entity /*self*/, World& /*world*/) {}

void BarrelSpawner::serializeFields(Serialization::ScriptFieldWriter& out) const
{
    out.setFloat("minX", static_cast<double>(m_minX));
    out.setFloat("maxX", static_cast<double>(m_maxX));
    out.setFloat("minY", static_cast<double>(m_minY));
    out.setFloat("maxY", static_cast<double>(m_maxY));
    out.setInt("barrelCount", static_cast<std::int64_t>(m_barrelCount));
    out.setBool("hasSpawned", m_hasSpawned);
}

void BarrelSpawner::deserializeFields(const Serialization::ScriptFieldReader& in)
{
    if (auto v = in.getFloat("minX"))
        m_minX = static_cast<float>(*v);
    if (auto v = in.getFloat("maxX"))
        m_maxX = static_cast<float>(*v);
    if (auto v = in.getFloat("minY"))
        m_minY = static_cast<float>(*v);
    if (auto v = in.getFloat("maxY"))
        m_maxY = static_cast<float>(*v);
    if (auto v = in.getInt("barrelCount"))
    {
        const std::int64_t c = *v;
        m_barrelCount        = (c < 0) ? 0u : static_cast<size_t>(c);
    }
    if (auto v = in.getBool("hasSpawned"))
        m_hasSpawned = *v;

    rebuildDistributions();
}

void BarrelSpawner::rebuildDistributions()
{
    // Avoid undefined behavior if the ranges are inverted.
    const float loX = std::min(m_minX, m_maxX);
    const float hiX = std::max(m_minX, m_maxX);
    const float loY = std::min(m_minY, m_maxY);
    const float hiY = std::max(m_minY, m_maxY);

    m_distX = std::uniform_real_distribution<float>(loX, hiX);
    m_distY = std::uniform_real_distribution<float>(loY, hiY);
}

}  // namespace Example
