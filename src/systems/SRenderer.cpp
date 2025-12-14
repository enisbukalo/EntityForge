#include "SRenderer.h"
#include <algorithm>
#include "CCollider2D.h"
#include "CMaterial.h"
#include "CParticleEmitter.h"
#include "CRenderable.h"
#include "CShader.h"
#include "CTexture.h"
#include "CTransform.h"
#include "Logger.h"
#include "SParticle.h"
#include "World.h"

namespace Systems
{

static sf::Color toSFMLColor(const Color& c)
{
    return sf::Color(c.r, c.g, c.b, c.a);
}

SRenderer::SRenderer() : System() {}

SRenderer::~SRenderer()
{
    shutdown();
}

bool SRenderer::initialize(const WindowConfig& config)
{
    if (m_initialized)
    {
        LOG_WARN("SRenderer: Already initialized");
        return true;
    }

    // Create the window
    m_window = std::make_unique<sf::RenderWindow>(sf::VideoMode(config.width, config.height),
                                                  config.title,
                                                  config.getStyleFlags(),
                                                  config.getContextSettings());

    // Check if window was created and is open
    if (!m_window->isOpen())
    {
        LOG_ERROR("SRenderer: Failed to create render window");
        m_window.reset();
        return false;
    }

    // Apply window settings
    m_window->setVerticalSyncEnabled(config.vsync);
    if (config.frameLimit > 0)
    {
        m_window->setFramerateLimit(config.frameLimit);
    }

    m_initialized = true;
    LOG_INFO("SRenderer: Initialized with window size {}x{}", config.width, config.height);
    return true;
}

void SRenderer::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    clearTextureCache();
    clearShaderCache();

    if (m_window)
    {
        m_window->close();
        m_window.reset();
    }

    m_initialized = false;
    LOG_INFO("SRenderer: Shutdown complete");
}

void SRenderer::update(float deltaTime, World& world)
{
    (void)deltaTime;
    (void)world;
    // Rendering system doesn't need per-frame updates
    // Actual rendering is done in render()
}

void SRenderer::render(World& world)
{
    if (!m_initialized || !m_window || !m_window->isOpen())
    {
        return;
    }

    struct RenderItem
    {
        Entity entity;
        int    zIndex;
        bool   isParticleEmitter;
    };

    std::vector<RenderItem> renderQueue;

    auto components = world.components();

    components.view2<::Components::CRenderable, ::Components::CTransform>(
        [&renderQueue](Entity entity, ::Components::CRenderable& renderable, ::Components::CTransform&)
        {
            if (!renderable.isVisible())
            {
                return;
            }
            renderQueue.push_back({entity, renderable.getZIndex(), false});
        });

    components.view2<::Components::CParticleEmitter, ::Components::CTransform>(
        [&renderQueue](Entity entity, ::Components::CParticleEmitter& emitter, ::Components::CTransform&)
        {
            if (emitter.isActive())
            {
                renderQueue.push_back({entity, emitter.getZIndex(), true});
            }
        });

    std::sort(renderQueue.begin(),
              renderQueue.end(),
              [](const RenderItem& a, const RenderItem& b) { return a.zIndex < b.zIndex; });

    for (const RenderItem& item : renderQueue)
    {
        if (item.isParticleEmitter)
        {
            if (m_particleSystem && m_particleSystem->isInitialized())
            {
                m_particleSystem->renderEmitter(item.entity, m_window.get(), world);
            }
            continue;
        }

        renderEntity(item.entity, world);
    }
}

void SRenderer::clear(const Color& color)
{
    if (m_window && m_window->isOpen())
    {
        m_window->clear(toSFMLColor(color));
    }
}

void SRenderer::display()
{
    if (m_window && m_window->isOpen())
    {
        m_window->display();
    }
}

bool SRenderer::isWindowOpen() const
{
    return m_window && m_window->isOpen();
}

sf::RenderWindow* SRenderer::getWindow()
{
    return m_window.get();
}

const sf::Texture* SRenderer::loadTexture(const std::string& filepath)
{
    if (filepath.empty())
    {
        return nullptr;
    }

    // Check if texture is already cached
    auto it = m_textureCache.find(filepath);
    if (it != m_textureCache.end())
    {
        return &it->second;
    }

    // Load new texture
    sf::Texture texture;
    if (!texture.loadFromFile(filepath))
    {
        LOG_ERROR("SRenderer: Failed to load texture from '{}'", filepath);
        return nullptr;
    }

    // Cache the texture
    m_textureCache[filepath] = std::move(texture);
    LOG_DEBUG("SRenderer: Loaded texture '{}'", filepath);
    return &m_textureCache[filepath];
}

const sf::Shader* SRenderer::loadShader(const std::string& vertexPath, const std::string& fragmentPath)
{
    if (vertexPath.empty() && fragmentPath.empty())
    {
        return nullptr;
    }

    // Create cache key from both paths
    std::string cacheKey = vertexPath + "|" + fragmentPath;

    // Check if shader is already cached
    auto it = m_shaderCache.find(cacheKey);
    if (it != m_shaderCache.end())
    {
        return it->second.get();
    }

    // Check if shaders are supported
    if (!sf::Shader::isAvailable())
    {
        LOG_WARN("SRenderer: Shaders are not available on this system");
        return nullptr;
    }

    // Load new shader
    auto shader = std::make_unique<sf::Shader>();
    bool loaded = false;

    if (!vertexPath.empty() && !fragmentPath.empty())
    {
        loaded = shader->loadFromFile(vertexPath, fragmentPath);
    }
    else if (!vertexPath.empty())
    {
        loaded = shader->loadFromFile(vertexPath, sf::Shader::Vertex);
    }
    else
    {
        loaded = shader->loadFromFile(fragmentPath, sf::Shader::Fragment);
    }

    if (!loaded)
    {
        LOG_ERROR("SRenderer: Failed to load shader (vertex: '{}', fragment: '{}')", vertexPath, fragmentPath);
        return nullptr;
    }

    // Cache the shader
    const sf::Shader* shaderPtr = shader.get();
    m_shaderCache[cacheKey]     = std::move(shader);
    LOG_DEBUG("SRenderer: Loaded shader (vertex: '{}', fragment: '{}')", vertexPath, fragmentPath);
    return shaderPtr;
}

void SRenderer::clearTextureCache()
{
    m_textureCache.clear();
    LOG_DEBUG("SRenderer: Texture cache cleared");
}

void SRenderer::clearShaderCache()
{
    m_shaderCache.clear();
    LOG_DEBUG("SRenderer: Shader cache cleared");
}

void SRenderer::renderEntity(Entity entity, World& world)
{
    if (!entity.isValid())
    {
        return;
    }

    auto components = world.components();

    // Check for required components
    auto* renderable = components.tryGet<::Components::CRenderable>(entity);
    auto* transform  = components.tryGet<::Components::CTransform>(entity);

    if (!renderable || !renderable->isVisible())
    {
        return;
    }

    if (!transform)
    {
        LOG_WARN("SRenderer: Entity has CRenderable but no CTransform");
        return;
    }

    // Get position, scale, and rotation
    Vec2  pos      = transform->getPosition();
    Vec2  scale    = transform->getScale();
    float rotation = transform->getRotation();

    // Convert from physics coordinates (meters, Y-up) to screen coordinates (pixels, Y-down)
    const float PIXELS_PER_METER = 100.0f;
    const float SCREEN_HEIGHT    = static_cast<float>(m_window->getSize().y);

    sf::Vector2f screenPos;
    screenPos.x = pos.x * PIXELS_PER_METER;
    screenPos.y = SCREEN_HEIGHT - (pos.y * PIXELS_PER_METER);  // Flip Y axis

    // Get material if available
    auto* material = components.tryGet<::Components::CMaterial>(entity);

    // Determine final color
    Color finalColor = renderable->getColor();
    if (material)
    {
        // Apply material tint
        Color tint   = material->getTint();
        finalColor.r = static_cast<uint8_t>((finalColor.r * tint.r) / 255);
        finalColor.g = static_cast<uint8_t>((finalColor.g * tint.g) / 255);
        finalColor.b = static_cast<uint8_t>((finalColor.b * tint.b) / 255);
        finalColor.a = static_cast<uint8_t>((finalColor.a * material->getOpacity()));
    }

    // Get texture if available (independent of material)
    const sf::Texture* texture = nullptr;
    {
        auto* textureComp = components.tryGet<::Components::CTexture>(entity);
        if (textureComp)
        {
            texture = loadTexture(textureComp->getTexturePath());
        }
    }

    // Get shader if available (independent of material)
    const sf::Shader* shader = nullptr;
    {
        auto* shaderComp = components.tryGet<::Components::CShader>(entity);
        if (shaderComp)
        {
            shader = loadShader(shaderComp->getVertexShaderPath(), shaderComp->getFragmentShaderPath());
        }
    }

    // Prepare render states
    sf::RenderStates states;
    if (material)
    {
        states.blendMode = toSFMLBlendMode(material->getBlendMode());
    }
    if (shader)
    {
        states.shader = shader;

        // Set common shader uniforms
        sf::Shader* mutableShader = const_cast<sf::Shader*>(shader);

        // Time uniform (elapsed time since program start)
        static sf::Clock shaderClock;
        float            time = shaderClock.getElapsedTime().asSeconds();
        mutableShader->setUniform("u_time", time);

        // Resolution uniform (screen dimensions)
        sf::Vector2u windowSize = m_window->getSize();
        mutableShader->setUniform("u_resolution",
                                  sf::Vector2f(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y)));
    }

    // Render based on visual type
    ::Components::VisualType visualType = renderable->getVisualType();

    switch (visualType)
    {
        case ::Components::VisualType::Rectangle:
        {
            sf::RectangleShape rect;

            // If we have a collider, use its size
            auto* collider = components.tryGet<::Components::CCollider2D>(entity);
            if (collider && collider->getShapeType() == ::Components::ColliderShape::Box)
            {
                float halfWidth  = collider->getBoxHalfWidth() * PIXELS_PER_METER;
                float halfHeight = collider->getBoxHalfHeight() * PIXELS_PER_METER;
                rect.setSize(sf::Vector2f(halfWidth * 2.0f, halfHeight * 2.0f));
                rect.setOrigin(halfWidth, halfHeight);
            }
            else
            {
                // Default rectangle size
                rect.setSize(sf::Vector2f(50.0f * scale.x, 50.0f * scale.y));
                rect.setOrigin(25.0f * scale.x, 25.0f * scale.y);
            }

            rect.setPosition(screenPos);
            rect.setRotation(-rotation * 180.0f / 3.14159265f);  // Negate for Y-axis flip
            rect.setFillColor(toSFMLColor(finalColor));

            if (texture)
            {
                rect.setTexture(texture);
            }

            m_window->draw(rect, states);
            break;
        }

        case ::Components::VisualType::Circle:
        {
            sf::CircleShape circle;

            // If we have a collider, use its radius
            auto* collider = components.tryGet<::Components::CCollider2D>(entity);
            float radius   = 25.0f;
            if (collider && collider->getShapeType() == ::Components::ColliderShape::Circle)
            {
                radius = collider->getCircleRadius() * PIXELS_PER_METER;
            }

            circle.setRadius(radius);
            circle.setOrigin(radius, radius);
            circle.setPosition(screenPos);
            circle.setScale(scale.x, scale.y);
            circle.setRotation(-rotation * 180.0f / 3.14159265f);  // Negate for Y-axis flip
            circle.setFillColor(toSFMLColor(finalColor));

            if (texture)
            {
                circle.setTexture(texture);
            }

            m_window->draw(circle, states);
            break;
        }

        case ::Components::VisualType::Sprite:
        {
            if (texture)
            {
                sf::Sprite    sprite(*texture);
                sf::FloatRect bounds = sprite.getLocalBounds();

                // Scale sprite to match physics collider size
                auto* collider = components.tryGet<::Components::CCollider2D>(entity);
                if (collider)
                {
                    float targetSize = 0.0f;
                    if (collider->getShapeType() == ::Components::ColliderShape::Circle)
                    {
                        // For circles, use diameter
                        targetSize = collider->getCircleRadius() * 2.0f * PIXELS_PER_METER;
                    }
                    else if (collider->getShapeType() == ::Components::ColliderShape::Box)
                    {
                        // For boxes, use width (assuming square-ish sprites)
                        targetSize = collider->getBoxHalfWidth() * 2.0f * PIXELS_PER_METER;
                    }
                    else if (collider->getShapeType() == ::Components::ColliderShape::Polygon)
                    {
                        // For polygon colliders (including multi-polygon bodies), calculate bounding box
                        float boundsWidth, boundsHeight;
                        if (collider->getBounds(boundsWidth, boundsHeight))
                        {
                            // Use the larger dimension to ensure the sprite covers the entire collider
                            targetSize = std::max(boundsWidth, boundsHeight) * PIXELS_PER_METER;
                        }
                    }

                    if (targetSize > 0.0f)
                    {
                        // Calculate scale to fit target size
                        // Use the smaller dimension to ensure sprite fills the collider
                        float spriteSize  = std::min(bounds.width, bounds.height);
                        float spriteScale = targetSize / spriteSize;
                        sprite.setScale(spriteScale * scale.x, spriteScale * scale.y);
                    }
                    else
                    {
                        sprite.setScale(scale.x, scale.y);
                    }
                }
                else
                {
                    sprite.setScale(scale.x, scale.y);
                }

                sprite.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
                sprite.setPosition(screenPos);
                sprite.setRotation(-rotation * 180.0f / 3.14159265f);  // Negate for Y-axis flip
                sprite.setColor(toSFMLColor(finalColor));

                m_window->draw(sprite, states);
            }
            else
            {
                // Fallback: draw a rectangle if no texture
                sf::RectangleShape rect(sf::Vector2f(50.0f * scale.x, 50.0f * scale.y));
                rect.setOrigin(25.0f * scale.x, 25.0f * scale.y);
                rect.setPosition(screenPos);
                rect.setRotation(-rotation * 180.0f / 3.14159265f);  // Negate for Y-axis flip
                rect.setFillColor(toSFMLColor(finalColor));
                m_window->draw(rect, states);
            }
            break;
        }

        case ::Components::VisualType::Line:
        {
            // Get line endpoints in local space
            Vec2 lineStart = renderable->getLineStart();
            Vec2 lineEnd   = renderable->getLineEnd();

            // Apply transform rotation to line endpoints
            float cosR = std::cos(rotation);
            float sinR = std::sin(rotation);

            Vec2 rotatedStart;
            rotatedStart.x = lineStart.x * cosR - lineStart.y * sinR;
            rotatedStart.y = lineStart.x * sinR + lineStart.y * cosR;

            Vec2 rotatedEnd;
            rotatedEnd.x = lineEnd.x * cosR - lineEnd.y * sinR;
            rotatedEnd.y = lineEnd.x * sinR + lineEnd.y * cosR;

            // Convert to screen coordinates (meters to pixels, Y-flip)
            sf::Vector2f screenStart;
            screenStart.x = screenPos.x + (rotatedStart.x * PIXELS_PER_METER * scale.x);
            screenStart.y = screenPos.y - (rotatedStart.y * PIXELS_PER_METER * scale.y);  // Negative for Y-flip

            sf::Vector2f screenEnd;
            screenEnd.x = screenPos.x + (rotatedEnd.x * PIXELS_PER_METER * scale.x);
            screenEnd.y = screenPos.y - (rotatedEnd.y * PIXELS_PER_METER * scale.y);  // Negative for Y-flip

            // Create line using VertexArray
            float           thickness = renderable->getLineThickness();
            sf::Color       lineColor = toSFMLColor(finalColor);
            sf::VertexArray line(sf::Lines, 2);
            line[0].position = screenStart;
            line[0].color    = lineColor;
            line[1].position = screenEnd;
            line[1].color    = lineColor;

            // Draw the line (for thickness > 1, draw multiple times with offset)
            if (thickness <= 1.0f)
            {
                m_window->draw(line, states);
            }
            else
            {
                // Calculate perpendicular offset for thickness
                sf::Vector2f direction = screenEnd - screenStart;
                float        length    = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length > 0.0f)
                {
                    sf::Vector2f perpendicular(-direction.y / length, direction.x / length);

                    // Draw multiple lines with offset to simulate thickness
                    int halfThickness = static_cast<int>(thickness / 2.0f);
                    for (int offset = -halfThickness; offset <= halfThickness; ++offset)
                    {
                        sf::VertexArray thickLine(sf::Lines, 2);
                        thickLine[0].position = screenStart + perpendicular * static_cast<float>(offset);
                        thickLine[0].color    = lineColor;
                        thickLine[1].position = screenEnd + perpendicular * static_cast<float>(offset);
                        thickLine[1].color    = lineColor;
                        m_window->draw(thickLine, states);
                    }
                }
            }
            break;
        }

        case ::Components::VisualType::Custom:
        case ::Components::VisualType::None:
        default:
            // No rendering for None or Custom (Custom would use shaders)
            break;
    }
}

sf::BlendMode SRenderer::toSFMLBlendMode(::Components::BlendMode blendMode) const
{
    switch (blendMode)
    {
        case ::Components::BlendMode::Add:
            return sf::BlendAdd;
        case ::Components::BlendMode::Multiply:
            return sf::BlendMultiply;
        case ::Components::BlendMode::None:
            return sf::BlendNone;
        case ::Components::BlendMode::Alpha:
        default:
            return sf::BlendAlpha;
    }
}

}  // namespace Systems
