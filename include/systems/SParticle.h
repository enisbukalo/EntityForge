#ifndef SPARTICLE_H
#define SPARTICLE_H

#include <Entity.h>
#include <Vec2.h>
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "System.h"

class Registry;  // Forward declaration

namespace Systems
{

/**
 * @brief Particle system that updates and renders particles from CParticleEmitter components
 *
 * @description
 * SParticle is responsible for updating particle physics and rendering
 * particles for all entities that have a CParticleEmitter component. It follows
 * the ECS pattern where the system operates on component data.
 *
 * Features:
 * - Updates particle physics (position, velocity, lifetime)
 * - Renders particles efficiently using SFML vertex arrays
 * - Supports textured and colored particles
 * - Automatic particle lifecycle management
 */
class SParticle : public System
{
public:
    SParticle();
    ~SParticle() override;

    /**
     * @brief Initializes the particle system
     * @param window SFML render window (used to get dimensions)
     * @param pixelsPerMeter Rendering scale (pixels per meter)
     * @return true if initialization succeeded
     */
    bool initialize(sf::RenderWindow* window, float pixelsPerMeter = 100.0f);

    /**
     * @brief Shuts down the particle system
     */
    void shutdown();

    /**
     * @brief Updates all particle emitters on entities
     * @param deltaTime Time elapsed since last update
     */
    void update(float deltaTime, World& world) override;

    /**
     * @brief Renders particles for a single emitter entity
     * @param entity Entity ID with CParticleEmitter component
     * @param window SFML render window
     * @param registry Registry to access components
     */
    void renderEmitter(Entity entity, sf::RenderWindow* window, World& world);

    /**
     * @brief Processes queued texture loads (once per frame).
     *
     * This is called by the renderer after it activates the window context.
     * It performs file IO and GPU uploads, so it must not be called in hot per-emitter code.
     */
    void processQueuedTextureLoads();

    /**
     * @brief Checks if the particle system is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const
    {
        return m_initialized;
    }

    std::string_view name() const override
    {
        return "SParticle";
    }

private:
    /** @brief Deleted copy constructor */
    SParticle(const SParticle&) = delete;

    /** @brief Deleted assignment operator */
    SParticle& operator=(const SParticle&) = delete;

    const sf::Texture* loadTexture(const std::string& filepath);
    void               requestTextureLoad(const std::string& filepath);
    const sf::Texture* getCachedTexture(const std::string& resolvedKey) const;

    sf::VertexArray   m_vertexArray;     ///< Vertex array for rendering
    sf::RenderWindow* m_window;          ///< Render window reference
    float             m_pixelsPerMeter;  ///< Rendering scale
    bool              m_initialized;     ///< Initialization state

    std::unordered_map<std::string, sf::Texture> m_textureCache;
    std::unordered_set<std::string>              m_queuedTextureLoads;
};

}  // namespace Systems

#endif  // SPARTICLESYSTEM_H
