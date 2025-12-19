#ifndef SRENDERER_H
#define SRENDERER_H

#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "Color.h"
#include "System.h"

class World;

namespace Components
{
enum class BlendMode;
}

namespace Systems
{

class SParticle;

/**
 * @brief Window initialization configuration
 */
struct WindowConfig
{
    unsigned int width        = 800;            ///< Window width in pixels
    unsigned int height       = 600;            ///< Window height in pixels
    std::string  title        = "Game Engine";  ///< Window title
    bool         fullscreen   = false;          ///< Fullscreen mode flag
    bool         vsync        = true;           ///< Vertical sync flag
    unsigned int frameLimit   = 0;              ///< Frame rate limit (0 = unlimited)
    unsigned int antialiasing = 0;              ///< Anti-aliasing level (0, 2, 4, 8, 16)

    /**
     * @brief Gets SFML window state (windowed vs fullscreen)
     * @return SFML state
     */
    sf::State getState() const
    {
        return fullscreen ? sf::State::Fullscreen : sf::State::Windowed;
    }

    /**
     * @brief Gets SFML window style flags based on configuration
     * @return SFML style flags
     */
    uint32_t getStyleFlags() const
    {
        // In SFML 3, fullscreen is expressed via sf::State at window creation time.
        // Style flags are decoration flags only.
        return fullscreen ? sf::Style::None : (sf::Style::Titlebar | sf::Style::Close);
    }

    /**
     * @brief Gets SFML context settings for window creation
     * @return SFML context settings
     */
    sf::ContextSettings getContextSettings() const
    {
        sf::ContextSettings settings;
        settings.antiAliasingLevel = antialiasing;
        return settings;
    }
};

/**
 * @brief Rendering system that manages SFML window and draws entities
 *
 * @description
 * SRenderer encapsulates SFML rendering functionality.
 * It owns and manages the render window internally, loads and caches textures
 * and shaders, and renders entities based on their components (CRenderable,
 * CMaterial, CTexture, CShader). The system provides an abstraction layer
 * over SFML for future portability to other rendering backends.
 *
 * Features:
 * - Internal window management
 * - Texture caching and loading
 * - Shader caching and compilation
 * - Z-index based layered rendering
 * - Fallback rendering for physics debug visualization
 */
class SRenderer : public System
{
public:
    SRenderer();
    ~SRenderer() override;

    std::string_view name() const override
    {
        return "SRenderer";
    }

    /**
     * @brief Initializes the renderer with window configuration
     * @param config Window initialization configuration
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize(const WindowConfig& config);

    /**
     * @brief Shuts down the renderer and releases resources
     */
    void shutdown();

    /**
     * @brief Updates the rendering system
     * @param deltaTime Time elapsed since last update (unused for rendering)
     *
     * This method is called each frame but doesn't perform actual rendering.
     * Call render() separately to draw the frame.
     */
    void update(float deltaTime, World& world) override;

    /**
     * @brief Renders all entities to the window
     *
     * Performs the following steps:
     * 1. Clears the window
     * 2. Sorts entities by z-index
     * 3. Draws each visible entity using its components
     * 4. Displays the frame
     */
    void render(World& world);

    /**
     * @brief Clears the window with a specified color
     * @param color Clear color (default: black)
     */
    void clear(const Color& color);

    /**
     * @brief Displays the rendered frame
     */
    void display();

    /**
     * @brief Checks if the window is open
     * @return true if window is open, false otherwise
     */
    bool isWindowOpen() const;

    /**
     * @brief Gets the SFML render window
     * @return Pointer to the render window
     *
     * Note: This is exposed for integration purposes (e.g., ImGui).
     * Direct rendering through this pointer bypasses the ECS abstraction.
     */
    sf::RenderWindow* getWindow();

    /**
     * @brief Loads a texture from file and caches it
     * @param filepath Path to the texture file
     * @return Pointer to the loaded texture, or nullptr on failure
     */
    const sf::Texture* loadTexture(const std::string& filepath);

    /**
     * @brief Enqueues a texture load request.
     *
     * This is safe to call from render code: it does not perform file IO or GPU uploads.
     * The actual load is processed once per frame inside SRenderer::render().
     */
    void requestTextureLoad(const std::string& filepath);

    /**
     * @brief Loads a shader from files and caches it
     * @param vertexPath Path to vertex shader (empty for no vertex shader)
     * @param fragmentPath Path to fragment shader (empty for no fragment shader)
     * @return Pointer to the loaded shader, or nullptr on failure
     */
    const sf::Shader* loadShader(const std::string& vertexPath, const std::string& fragmentPath);

    /**
     * @brief Clears the texture cache
     */
    void clearTextureCache();

    /**
     * @brief Clears the shader cache
     */
    void clearShaderCache();

    /**
     * @brief Inject particle system for particle rendering
     */
    void setParticleSystem(SParticle* particleSystem)
    {
        m_particleSystem = particleSystem;
    }

private:
    /** @brief Deleted copy constructor */
    SRenderer(const SRenderer&) = delete;

    /** @brief Deleted assignment operator */
    SRenderer& operator=(const SRenderer&) = delete;

    /**
     * @brief Renders a single entity
     * @param entity Entity ID to render
     * @param registry Registry to access components
     */
    void renderEntity(Entity entity, World& world);

    /**
     * @brief Converts engine BlendMode to SFML BlendMode
     * @param blendMode Engine blend mode
     * @return Equivalent SFML blend mode
     */
    sf::BlendMode toSFMLBlendMode(::Components::BlendMode blendMode) const;

    const sf::Texture* getCachedTexture(const std::string& resolvedKey) const;
    void               processQueuedTextureLoads();

    std::unique_ptr<sf::RenderWindow>            m_window;                       ///< The render window
    std::unordered_map<std::string, sf::Texture> m_textureCache;                 ///< Cached textures by filepath
    std::unordered_map<std::string, std::unique_ptr<sf::Shader>> m_shaderCache;  ///< Cached shaders by filepath combination
    std::unordered_set<std::string>              m_queuedTextureLoads;           ///< Resolved cache keys queued for load
    bool       m_initialized    = false;                                         ///< Initialization state
    SParticle* m_particleSystem = nullptr;                                       ///< Optional particle system hookup
};

}  // namespace Systems

#endif  // SRENDERER_H
