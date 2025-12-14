#include "SRenderer.h"
#include <algorithm>
#include <vector>
#include "CCamera.h"
#include "CCollider2D.h"
#include "CMaterial.h"
#include "CParticleEmitter.h"
#include "CRenderable.h"
#include "CShader.h"
#include "CTexture.h"
#include "CTransform.h"
#include "CameraView.h"
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

    struct CameraItem
    {
        Entity                 entity;
        ::Components::CCamera* camera;
    };

    std::vector<CameraItem> cameras;
    components.view<::Components::CCamera>([&cameras](Entity entity, ::Components::CCamera& camera)
                                           { cameras.push_back({entity, &camera}); });

    std::stable_sort(cameras.begin(),
                     cameras.end(),
                     [](const CameraItem& a, const CameraItem& b)
                     {
                         if (a.camera->renderOrder != b.camera->renderOrder)
                         {
                             return a.camera->renderOrder < b.camera->renderOrder;
                         }
                         return a.entity < b.entity;
                     });

    // If no cameras exist, fall back to a default camera so the renderer still uses world units.
    ::Components::CCamera defaultCamera;
    if (cameras.empty())
    {
        cameras.push_back({Entity::null(), &defaultCamera});
    }

    for (const CameraItem& cameraItem : cameras)
    {
        const ::Components::CCamera& camera = *cameraItem.camera;
        if (!camera.enabled || !camera.render)
        {
            continue;
        }

        const sf::View view = Internal::buildViewFromCamera(camera, m_window->getSize());
        m_window->setView(view);

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

    // World-space transform (meters, Y-up). View handles Y-up mapping.
    Vec2  pos      = transform->getPosition();
    Vec2  scale    = transform->getScale();
    float rotation = transform->getRotation();

    constexpr float kPixelsPerMeter = 100.0f;
    constexpr float kPi             = 3.14159265358979323846f;
    // CTransform stores radians in a Y-up world (CCW-positive). Since the active sf::View
    // already maps the world into SFML's space, we can pass the angle through directly.
    const float rotationDegrees = rotation * 180.0f / kPi;

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
                float halfWidth  = collider->getBoxHalfWidth();
                float halfHeight = collider->getBoxHalfHeight();
                rect.setSize(sf::Vector2f(halfWidth * 2.0f, halfHeight * 2.0f));
                rect.setOrigin(halfWidth, halfHeight);
            }
            else
            {
                // Default rectangle size (pixels converted to meters)
                constexpr float kDefaultSizeM = 50.0f / kPixelsPerMeter;
                rect.setSize(sf::Vector2f(kDefaultSizeM * scale.x, kDefaultSizeM * scale.y));
                rect.setOrigin((kDefaultSizeM * 0.5f) * scale.x, (kDefaultSizeM * 0.5f) * scale.y);
            }

            rect.setPosition(pos.x, pos.y);
            rect.setRotation(rotationDegrees);
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
                radius = collider->getCircleRadius();
            }
            else
            {
                // Default radius (pixels converted to meters)
                radius = 25.0f / kPixelsPerMeter;
            }

            circle.setRadius(radius);
            circle.setOrigin(radius, radius);
            circle.setPosition(pos.x, pos.y);
            circle.setScale(scale.x, scale.y);
            circle.setRotation(rotationDegrees);
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

                // Align sprite to the physics collider by using the collider's local bounds.
                // This ensures CTransform.position acts as the shared origin for physics + rendering.
                auto* collider = components.tryGet<::Components::CCollider2D>(entity);
                if (collider && !collider->fixtures.empty())
                {
                    float minX = std::numeric_limits<float>::infinity();
                    float minY = std::numeric_limits<float>::infinity();
                    float maxX = -std::numeric_limits<float>::infinity();
                    float maxY = -std::numeric_limits<float>::infinity();

                    auto includePoint = [&](const Vec2& p)
                    {
                        minX = std::min(minX, p.x);
                        minY = std::min(minY, p.y);
                        maxX = std::max(maxX, p.x);
                        maxY = std::max(maxY, p.y);
                    };

                    for (const auto& f : collider->fixtures)
                    {
                        switch (f.shapeType)
                        {
                            case ::Components::ColliderShape::Circle:
                            {
                                const Vec2  c = f.circle.center;
                                const float r = f.circle.radius;
                                includePoint(Vec2{c.x - r, c.y - r});
                                includePoint(Vec2{c.x + r, c.y + r});
                                break;
                            }
                            case ::Components::ColliderShape::Box:
                            {
                                const float hw = f.box.halfWidth;
                                const float hh = f.box.halfHeight;
                                includePoint(Vec2{-hw, -hh});
                                includePoint(Vec2{hw, hh});
                                break;
                            }
                            case ::Components::ColliderShape::Polygon:
                            {
                                for (const auto& v : f.polygon.vertices)
                                {
                                    includePoint(v);
                                }
                                break;
                            }
                            case ::Components::ColliderShape::Segment:
                            {
                                includePoint(f.segment.point1);
                                includePoint(f.segment.point2);
                                break;
                            }
                            case ::Components::ColliderShape::ChainSegment:
                            {
                                includePoint(f.chainSegment.ghost1);
                                includePoint(f.chainSegment.point1);
                                includePoint(f.chainSegment.point2);
                                includePoint(f.chainSegment.ghost2);
                                break;
                            }
                        }
                    }

                    const float worldWidth  = maxX - minX;
                    const float worldHeight = maxY - minY;

                    const float texWidthPx  = std::max(1.0f, bounds.width);
                    const float texHeightPx = std::max(1.0f, bounds.height);

                    if (std::isfinite(worldWidth) && std::isfinite(worldHeight) && worldWidth > 0.0f && worldHeight > 0.0f)
                    {
                        // Use a uniform scale based on the collider's max dimension. This matches the old
                        // behavior better for assets with asymmetric aspect ratios or transparent padding.
                        const float targetWorld  = std::max(worldWidth, worldHeight);
                        const float spriteSizePx = std::max(1.0f, std::min(texWidthPx, texHeightPx));
                        const float uniformScale = targetWorld / spriteSizePx;
                        sprite.setScale(uniformScale * scale.x, uniformScale * scale.y);

                        // Map the body origin (0,0) into the collider bounds and use that as the sprite origin.
                        const float originXPx = bounds.left + ((0.0f - minX) / worldWidth) * texWidthPx;
                        const float originYPx = bounds.top + ((0.0f - minY) / worldHeight) * texHeightPx;
                        sprite.setOrigin(originXPx, originYPx);
                    }
                    else
                    {
                        const float baseScale = 1.0f / kPixelsPerMeter;
                        sprite.setScale(baseScale * scale.x, baseScale * scale.y);
                        sprite.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
                    }
                }
                else
                {
                    const float baseScale = 1.0f / kPixelsPerMeter;
                    sprite.setScale(baseScale * scale.x, baseScale * scale.y);
                    sprite.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
                }

                sprite.setPosition(pos.x, pos.y);
                // Convention: +Y is forward at 0 radians. Most existing Example textures were authored
                // "forward" pointing down (screen-space), so apply a 180° flip at render time.
                sprite.setRotation(rotationDegrees + 180.0f);
                sprite.setColor(toSFMLColor(finalColor));

                m_window->draw(sprite, states);
            }
            else
            {
                // Fallback: draw a rectangle if no texture
                constexpr float    kDefaultSizeM = 50.0f / kPixelsPerMeter;
                sf::RectangleShape rect(sf::Vector2f(kDefaultSizeM * scale.x, kDefaultSizeM * scale.y));
                rect.setOrigin((kDefaultSizeM * 0.5f) * scale.x, (kDefaultSizeM * 0.5f) * scale.y);
                rect.setPosition(pos.x, pos.y);
                rect.setRotation(rotationDegrees);
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
            sf::Vector2f worldStart;
            worldStart.x = pos.x + (rotatedStart.x * scale.x);
            worldStart.y = pos.y + (rotatedStart.y * scale.y);

            sf::Vector2f worldEnd;
            worldEnd.x = pos.x + (rotatedEnd.x * scale.x);
            worldEnd.y = pos.y + (rotatedEnd.y * scale.y);

            // Create line using VertexArray
            float           thickness = renderable->getLineThickness();
            sf::Color       lineColor = toSFMLColor(finalColor);
            sf::VertexArray line(sf::Lines, 2);
            line[0].position = worldStart;
            line[0].color    = lineColor;
            line[1].position = worldEnd;
            line[1].color    = lineColor;

            // Draw the line (for thickness > 1, draw multiple times with offset)
            if (thickness <= 1.0f)
            {
                m_window->draw(line, states);
            }
            else
            {
                // Calculate perpendicular offset for thickness
                sf::Vector2f direction = worldEnd - worldStart;
                float        length    = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                if (length > 0.0f)
                {
                    sf::Vector2f perpendicular(-direction.y / length, direction.x / length);

                    // Draw multiple lines with offset to simulate thickness
                    int halfThickness = static_cast<int>(thickness / 2.0f);
                    for (int offset = -halfThickness; offset <= halfThickness; ++offset)
                    {
                        sf::VertexArray thickLine(sf::Lines, 2);
                        thickLine[0].position = worldStart + perpendicular * static_cast<float>(offset);
                        thickLine[0].color    = lineColor;
                        thickLine[1].position = worldEnd + perpendicular * static_cast<float>(offset);
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
