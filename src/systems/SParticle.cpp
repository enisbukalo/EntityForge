#include "SParticle.h"
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <random>
#include <sstream>
#include "CParticleEmitter.h"
#include "CTransform.h"
#include "ExecutablePaths.h"
#include "FileUtilities.h"
#include "Logger.h"
#include "Registry.h"
#include "World.h"

namespace Systems
{

const sf::Texture* SParticle::loadTexture(const std::string& filepath)
{
    if (filepath.empty())
    {
        return nullptr;
    }

    static uint64_t s_textureLoadAttempt = 0;
    const uint64_t  loadAttempt          = s_textureLoadAttempt++;

    const std::filesystem::path resolvedPath = Internal::ExecutablePaths::resolveRelativeToExecutableDir(filepath);
    const std::string          resolvedStr  = resolvedPath.string();

    if (loadAttempt < 6)
    {
        std::error_code ec;
        const auto      cwd    = std::filesystem::current_path(ec);
        const auto      exeDir = Internal::ExecutablePaths::getExecutableDir();
        LOG_INFO("SParticle::loadTexture attempt {} filepath='{}' resolved='{}' cwd='{}' exeDir='{}'",
                 loadAttempt,
                 filepath,
                 resolvedStr,
                 ec ? std::string("<error>") : cwd.string(),
                 exeDir.string());
    }

    auto it = m_textureCache.find(resolvedStr);
    if (it != m_textureCache.end())
    {
        return &it->second;
    }

    {
        std::error_code ec;
        const bool      exists = std::filesystem::exists(resolvedPath, ec);
        if (ec)
        {
            LOG_WARN("SParticle::loadTexture: exists() error for '{}' : {}", resolvedStr, ec.message());
        }
        if (!exists)
        {
            LOG_WARN("SParticle::loadTexture: file does not exist: '{}' (original='{}')", resolvedStr, filepath);
            return nullptr;
        }
    }

    if (!m_window || !m_window->isOpen())
    {
        return nullptr;
    }

    // Ensure the window context is active before creating/uploading textures.
    if (!m_window->setActive(true))
    {
        LOG_WARN("SParticle::loadTexture: setActive(true) failed, skipping load for '{}'", resolvedStr);
        return nullptr;
    }

    sf::Texture texture;
    bool        loaded = false;
    try
    {
        loaded = texture.loadFromFile(resolvedPath);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("SParticle::loadTexture: exception loading '{}': {}", resolvedStr, e.what());
        loaded = false;
    }

    if (!loaded)
    {
        LOG_WARN("SParticle::loadTexture: failed to load '{}'", resolvedStr);
        return nullptr;
    }

    auto [insertedIt, inserted] = m_textureCache.emplace(resolvedStr, std::move(texture));
    (void)inserted;
    return &insertedIt->second;

}


const sf::Texture* SParticle::getCachedTexture(const std::string& resolvedKey) const
{
    auto it = m_textureCache.find(resolvedKey);
    if (it == m_textureCache.end())
    {
        return nullptr;
    }
    return &it->second;
}

void SParticle::requestTextureLoad(const std::string& filepath)
{
    if (filepath.empty())
    {
        return;
    }

    const std::filesystem::path resolvedPath = Internal::ExecutablePaths::resolveRelativeToExecutableDir(filepath);
    const std::string          resolvedStr  = resolvedPath.string();

    if (resolvedStr.empty())
    {
        return;
    }

    if (m_textureCache.find(resolvedStr) != m_textureCache.end())
    {
        return;
    }

    m_queuedTextureLoads.insert(resolvedStr);
}

void SParticle::processQueuedTextureLoads()
{
    if (!m_window || !m_window->isOpen())
    {
        return;
    }

    if (m_queuedTextureLoads.empty())
    {
        return;
    }

    // Activate once per batch.
    if (!m_window->setActive(true))
    {
        LOG_WARN("SParticle::processQueuedTextureLoads: setActive(true) failed, deferring {} queued textures",
                 m_queuedTextureLoads.size());
        return;
    }

    // Limit work per frame.
    constexpr size_t kMaxLoadsPerFrame = 2;
    size_t          loadsThisFrame     = 0;

    static uint64_t s_debugLoadsLogged = 0;

    for (auto it = m_queuedTextureLoads.begin(); it != m_queuedTextureLoads.end() && loadsThisFrame < kMaxLoadsPerFrame;)
    {
        const std::string& resolvedStr = *it;

        const bool logThis = s_debugLoadsLogged < 32;
        if (logThis)
        {
            LOG_INFO("SParticle::processQueuedTextureLoads: begin '{}'", resolvedStr);
            ++s_debugLoadsLogged;
        }

        if (m_textureCache.find(resolvedStr) != m_textureCache.end())
        {
            it = m_queuedTextureLoads.erase(it);
            continue;
        }

        const std::filesystem::path resolvedPath(resolvedStr);
        std::error_code             ec;
        const bool                  exists = std::filesystem::exists(resolvedPath, ec);
        if (ec)
        {
            LOG_WARN("SParticle::processQueuedTextureLoads: exists() error for '{}' : {}", resolvedStr, ec.message());
        }
        if (!exists)
        {
            LOG_WARN("SParticle::processQueuedTextureLoads: file does not exist: '{}'", resolvedStr);
            it = m_queuedTextureLoads.erase(it);
            continue;
        }

        if (logThis)
        {
            LOG_INFO("SParticle::processQueuedTextureLoads: reading bytes for '{}'", resolvedStr);
        }

        std::vector<std::uint8_t> fileBytes;
        try
        {
            fileBytes = Internal::FileUtilities::readFileBinary(resolvedPath);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("SParticle::processQueuedTextureLoads: exception reading '{}': {}", resolvedStr, e.what());
            it = m_queuedTextureLoads.erase(it);
            continue;
        }

        if (logThis)
        {
            std::ostringstream sig;
            sig << std::hex << std::setfill('0');
            const size_t sigBytes = std::min<size_t>(8, fileBytes.size());
            for (size_t i = 0; i < sigBytes; ++i)
            {
                sig << std::setw(2) << static_cast<unsigned int>(fileBytes[i]);
                if (i + 1 < sigBytes)
                {
                    sig << ' ';
                }
            }
            LOG_INFO("SParticle::processQueuedTextureLoads: read {} bytes (sig={}) for '{}'",
                     fileBytes.size(),
                     sig.str(),
                     resolvedStr);
            LOG_INFO("SParticle::processQueuedTextureLoads: decoding via sf::Image::loadFromMemory for '{}'", resolvedStr);
        }

        sf::Image image;
        bool      imageLoaded = false;
        try
        {
            imageLoaded = image.loadFromMemory(fileBytes.data(), fileBytes.size());
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("SParticle::processQueuedTextureLoads: exception decoding '{}': {}", resolvedStr, e.what());
            imageLoaded = false;
        }

        if (logThis)
        {
            LOG_INFO("SParticle::processQueuedTextureLoads: Image::loadFromMemory returned {} for '{}'",
                     imageLoaded ? "true" : "false",
                     resolvedStr);
        }

        if (!imageLoaded)
        {
            LOG_WARN("SParticle::processQueuedTextureLoads: decode failed for '{}'", resolvedStr);
            it = m_queuedTextureLoads.erase(it);
            continue;
        }

        if (logThis)
        {
            LOG_INFO("SParticle::processQueuedTextureLoads: uploading via sf::Texture::loadFromImage for '{}'", resolvedStr);
        }

        sf::Texture texture;
        bool        loaded = false;
        try
        {
            loaded = texture.loadFromImage(image);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("SParticle::processQueuedTextureLoads: exception uploading '{}': {}", resolvedStr, e.what());
            loaded = false;
        }

        if (logThis)
        {
            LOG_INFO("SParticle::processQueuedTextureLoads: Texture::loadFromImage returned {} for '{}'",
                     loaded ? "true" : "false",
                     resolvedStr);
        }

        if (!loaded)
        {
            LOG_WARN("SParticle::processQueuedTextureLoads: upload failed for '{}'", resolvedStr);
            it = m_queuedTextureLoads.erase(it);
            continue;
        }

        m_textureCache.emplace(resolvedStr, std::move(texture));
        it = m_queuedTextureLoads.erase(it);
        ++loadsThisFrame;
    }
}

static std::random_device               s_rd;
static std::mt19937                     s_gen(s_rd());
static std::uniform_real_distribution<> s_dist(0.0, 1.0);

// Helper function to generate random float
static float randomFloat(float min, float max)
{
    return min + static_cast<float>(s_dist(s_gen)) * (max - min);
}

// Helper function to interpolate between two values
template <typename T>
static T lerp(const T& a, const T& b, float t)
{
    return a + (b - a) * t;
}

// Helper function to interpolate between two colors
static Color lerpColor(const Color& a, const Color& b, float t)
{
    return Color(static_cast<unsigned char>(lerp(static_cast<float>(a.r), static_cast<float>(b.r), t)),
                 static_cast<unsigned char>(lerp(static_cast<float>(a.g), static_cast<float>(b.g), t)),
                 static_cast<unsigned char>(lerp(static_cast<float>(a.b), static_cast<float>(b.b), t)),
                 static_cast<unsigned char>(lerp(static_cast<float>(a.a), static_cast<float>(b.a), t)));
}

//=============================================================================
// Shape-based emission helpers
//=============================================================================

/**
 * @brief Sample a position on the edge of a circle
 * @param radius Circle radius
 * @return Position on circle edge and outward normal
 */
static std::pair<Vec2, Vec2> sampleCircleEdge(float radius)
{
    float angle = randomFloat(0.0f, 2.0f * 3.14159f);
    Vec2  position(std::cos(angle) * radius, std::sin(angle) * radius);
    Vec2  normal(std::cos(angle), std::sin(angle));  // Outward normal
    return {position, normal};
}

/**
 * @brief Sample a position on the edge of a rectangle
 * @param halfWidth Half-width of the rectangle
 * @param halfHeight Half-height of the rectangle
 * @return Position on rectangle edge and outward normal
 */
static std::pair<Vec2, Vec2> sampleRectangleEdge(float halfWidth, float halfHeight)
{
    // Calculate perimeter for uniform distribution
    float perimeter = 2.0f * (2.0f * halfWidth + 2.0f * halfHeight);
    float t         = randomFloat(0.0f, perimeter);

    float topLength    = 2.0f * halfWidth;
    float rightLength  = 2.0f * halfHeight;
    float bottomLength = 2.0f * halfWidth;

    Vec2 position;
    Vec2 normal;

    if (t < topLength)
    {
        // Top edge
        position = Vec2(-halfWidth + t, halfHeight);
        normal   = Vec2(0.0f, 1.0f);
    }
    else if (t < topLength + rightLength)
    {
        // Right edge
        float localT = t - topLength;
        position     = Vec2(halfWidth, halfHeight - localT);
        normal       = Vec2(1.0f, 0.0f);
    }
    else if (t < topLength + rightLength + bottomLength)
    {
        // Bottom edge
        float localT = t - topLength - rightLength;
        position     = Vec2(halfWidth - localT, -halfHeight);
        normal       = Vec2(0.0f, -1.0f);
    }
    else
    {
        // Left edge
        float localT = t - topLength - rightLength - bottomLength;
        position     = Vec2(-halfWidth, -halfHeight + localT);
        normal       = Vec2(-1.0f, 0.0f);
    }

    return {position, normal};
}

/**
 * @brief Sample a position along a line segment
 * @param start Start point of the line
 * @param end End point of the line
 * @return Position on line and perpendicular normal
 */
static std::pair<Vec2, Vec2> sampleLine(const Vec2& start, const Vec2& end)
{
    float t        = randomFloat(0.0f, 1.0f);
    Vec2  position = lerp(start, end, t);

    // Calculate perpendicular normal (90 degrees rotated from line direction)
    Vec2  direction(end.x - start.x, end.y - start.y);
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length > 0.0001f)
    {
        direction.x /= length;
        direction.y /= length;
    }
    // Perpendicular: rotate 90 degrees counter-clockwise
    Vec2 normal(-direction.y, direction.x);

    return {position, normal};
}

/**
 * @brief Sample a position on the edge of a polygon
 * @param vertices Polygon vertices (should form a closed shape)
 * @return Position on polygon edge and outward normal
 */
static std::pair<Vec2, Vec2> samplePolygonEdge(const std::vector<Vec2>& vertices)
{
    if (vertices.size() < 2)
    {
        return {Vec2(0.0f, 0.0f), Vec2(0.0f, 1.0f)};
    }

    // Calculate total perimeter
    float              totalPerimeter = 0.0f;
    std::vector<float> edgeLengths;
    size_t             numVertices = vertices.size();

    for (size_t i = 0; i < numVertices; ++i)
    {
        const Vec2& v1     = vertices[i];
        const Vec2& v2     = vertices[(i + 1) % numVertices];
        float       dx     = v2.x - v1.x;
        float       dy     = v2.y - v1.y;
        float       length = std::sqrt(dx * dx + dy * dy);
        edgeLengths.push_back(length);
        totalPerimeter += length;
    }

    if (totalPerimeter < 0.0001f)
    {
        return {vertices[0], Vec2(0.0f, 1.0f)};
    }

    // Random position along perimeter
    float targetDist  = randomFloat(0.0f, totalPerimeter);
    float accumulated = 0.0f;

    for (size_t i = 0; i < numVertices; ++i)
    {
        if (accumulated + edgeLengths[i] >= targetDist)
        {
            // This is the edge
            float       localT = (targetDist - accumulated) / edgeLengths[i];
            const Vec2& v1     = vertices[i];
            const Vec2& v2     = vertices[(i + 1) % numVertices];

            Vec2 position = lerp(v1, v2, localT);

            // Calculate outward normal (perpendicular to edge, pointing outward)
            // For counter-clockwise winding, the RIGHT perpendicular points outward
            // For clockwise winding, the LEFT perpendicular points outward
            Vec2  direction(v2.x - v1.x, v2.y - v1.y);
            float length = edgeLengths[i];
            if (length > 0.0001f)
            {
                direction.x /= length;
                direction.y /= length;
            }
            // Right perpendicular (outward for CCW winding in Y-up coordinate system)
            Vec2 normal(direction.y, -direction.x);

            return {position, normal};
        }
        accumulated += edgeLengths[i];
    }

    // Fallback
    return {vertices[0], Vec2(0.0f, 1.0f)};
}

/**
 * @brief Get emission position and optional outward direction based on emission shape
 * @param emitter The particle emitter component
 * @param entityRotation Rotation of the entity in radians
 * @return Local-space position offset and outward normal direction
 */
static std::pair<Vec2, Vec2> getEmissionPositionAndNormal(const ::Components::CParticleEmitter* emitter, float entityRotation)
{
    Vec2 localPosition(0.0f, 0.0f);
    Vec2 outwardNormal(0.0f, 1.0f);

    switch (emitter->getEmissionShape())
    {
        case ::Components::EmissionShape::Point:
            // Point emission - no position offset, use configured direction
            break;

        case ::Components::EmissionShape::Circle:
        {
            auto [pos, normal] = sampleCircleEdge(emitter->getShapeRadius());
            localPosition      = pos;
            outwardNormal      = normal;
            break;
        }

        case ::Components::EmissionShape::Rectangle:
        {
            Vec2 size          = emitter->getShapeSize();
            auto [pos, normal] = sampleRectangleEdge(size.x / 2.0f, size.y / 2.0f);
            localPosition      = pos;
            outwardNormal      = normal;
            break;
        }

        case ::Components::EmissionShape::Line:
        {
            auto [pos, normal] = sampleLine(emitter->getLineStart(), emitter->getLineEnd());
            localPosition      = pos;
            outwardNormal      = normal;
            break;
        }

        case ::Components::EmissionShape::Polygon:
        {
            const auto& vertices = emitter->getPolygonVertices();
            if (!vertices.empty())
            {
                auto [pos, normal] = samplePolygonEdge(vertices);
                localPosition      = pos;
                outwardNormal      = normal;
            }
            break;
        }
    }

    // Rotate local position and normal by entity rotation
    if (entityRotation != 0.0f)
    {
        float cosR = std::cos(entityRotation);
        float sinR = std::sin(entityRotation);

        float rotatedPosX = localPosition.x * cosR - localPosition.y * sinR;
        float rotatedPosY = localPosition.x * sinR + localPosition.y * cosR;
        localPosition     = Vec2(rotatedPosX, rotatedPosY);

        float rotatedNormX = outwardNormal.x * cosR - outwardNormal.y * sinR;
        float rotatedNormY = outwardNormal.x * sinR + outwardNormal.y * cosR;
        outwardNormal      = Vec2(rotatedNormX, rotatedNormY);
    }

    return {localPosition, outwardNormal};
}

//=============================================================================
// ::Components::Particle emission and update logic
//=============================================================================

/**
 * @brief Check if emission should be allowed based on velocity direction
 * @param outwardNormal The outward normal of the emission point (world space)
 * @param velocity The velocity of the emitter (world space)
 * @return True if emission should proceed, false if filtered out
 */
static bool shouldEmitAtNormal(const Vec2& outwardNormal, const Vec2& velocity)
{
    // Calculate dot product: positive = facing same direction as velocity
    // We want to emit from edges facing OPPOSITE to velocity (behind the object)
    float dot = outwardNormal.x * velocity.x + outwardNormal.y * velocity.y;

    // Only emit if normal is facing opposite to velocity direction (dot < 0)
    // This means the edge is on the "trailing" side of the object
    return dot < 0.0f;
}

static ::Components::Particle spawnParticle(const ::Components::CParticleEmitter* emitter, const Vec2& worldPosition, float entityRotation)
{
    ::Components::Particle p;
    p.alive = true;
    p.age   = 0.0f;

    // Get emission position and outward normal based on shape
    auto [shapeOffset, outwardNormal] = getEmissionPositionAndNormal(emitter, entityRotation);

    // Position: world position + shape-based offset
    p.position = Vec2(worldPosition.x + shapeOffset.x, worldPosition.y + shapeOffset.y);

    // Lifetime
    p.lifetime = randomFloat(emitter->getMinLifetime(), emitter->getMaxLifetime());

    // Size
    p.size        = randomFloat(emitter->getMinSize(), emitter->getMaxSize());
    p.initialSize = p.size;

    // Velocity (direction + spread + speed)
    Vec2 direction;

    if (emitter->getEmitOutward() && emitter->getEmissionShape() != ::Components::EmissionShape::Point)
    {
        // Emit in direction away from shape center (use outward normal)
        direction = outwardNormal;
    }
    else
    {
        // Use configured emission direction
        direction = emitter->getDirection();

        // Rotate the emission direction by entity rotation for local-space emission
        if (entityRotation != 0.0f)
        {
            float cosR     = std::cos(entityRotation);
            float sinR     = std::sin(entityRotation);
            float rotatedX = direction.x * cosR - direction.y * sinR;
            float rotatedY = direction.x * sinR + direction.y * cosR;
            direction      = Vec2(rotatedX, rotatedY);
        }
    }

    float angle  = std::atan2(direction.y, direction.x);
    float spread = randomFloat(-emitter->getSpreadAngle(), emitter->getSpreadAngle());
    angle += spread;

    float speed = randomFloat(emitter->getMinSpeed(), emitter->getMaxSpeed());
    p.velocity  = Vec2(std::cos(angle) * speed, std::sin(angle) * speed);

    // Acceleration (gravity)
    p.acceleration = emitter->getGravity();

    // Color and alpha
    p.color = emitter->getStartColor();
    p.alpha = emitter->getStartAlpha();

    // Rotation
    p.rotation      = randomFloat(0.0f, 2.0f * 3.14159f);
    p.rotationSpeed = randomFloat(emitter->getMinRotationSpeed(), emitter->getMaxRotationSpeed());

    return p;
}

static void updateParticle(::Components::Particle& particle, const ::Components::CParticleEmitter* emitter, float deltaTime)
{
    particle.age += deltaTime;

    // Kill if lifetime exceeded
    if (particle.age >= particle.lifetime)
    {
        particle.alive = false;
        return;
    }

    // Update physics
    particle.velocity.x += particle.acceleration.x * deltaTime;
    particle.velocity.y += particle.acceleration.y * deltaTime;
    particle.position.x += particle.velocity.x * deltaTime;
    particle.position.y += particle.velocity.y * deltaTime;

    // Update rotation
    particle.rotation += particle.rotationSpeed * deltaTime;

    // Interpolation factor (0 at birth, 1 at death)
    float t = particle.age / particle.lifetime;

    // Color interpolation
    particle.color = lerpColor(emitter->getStartColor(), emitter->getEndColor(), t);

    // Alpha fade
    if (emitter->getFadeOut())
    {
        particle.alpha = lerp(emitter->getStartAlpha(), emitter->getEndAlpha(), t);
    }

    // Size shrink
    if (emitter->getShrink())
    {
        particle.size = particle.initialSize * lerp(1.0f, emitter->getShrinkEndScale(), t);
    }
}

static void emitParticle(::Components::CParticleEmitter* emitter, const Vec2& worldPosition, float entityRotation)
{
    // Check particle limit
    if (emitter->getAliveCount() >= static_cast<size_t>(emitter->getMaxParticles()))
    {
        return;
    }

    // Find dead particle to reuse or add new one
    auto& particles = emitter->getParticles();
    auto it = std::find_if(particles.begin(), particles.end(), [](const ::Components::Particle& p) { return !p.alive; });
    if (it != particles.end())
    {
        *it = spawnParticle(emitter, worldPosition, entityRotation);
        return;
    }

    // No dead particles, add new one
    emitter->getParticles().push_back(spawnParticle(emitter, worldPosition, entityRotation));
}

SParticle::SParticle() : m_vertexArray(sf::PrimitiveType::Triangles), m_window(nullptr), m_pixelsPerMeter(100.0f), m_initialized(false) {}

SParticle::~SParticle()
{
    shutdown();
}

bool SParticle::initialize(sf::RenderWindow* window, float pixelsPerMeter)
{
    m_window         = window;
    m_pixelsPerMeter = pixelsPerMeter;
    m_initialized    = true;
    LOG_INFO("SParticle: Initialized with pixelsPerMeter={}", pixelsPerMeter);
    return true;
}

void SParticle::shutdown()
{
    m_initialized = false;
    m_window      = nullptr;
    LOG_INFO("SParticle: Shutdown complete");
}

void SParticle::update(float deltaTime, World& world)
{
    if (m_initialized == false)
    {
        return;
    }

    world.components().view2<::Components::CParticleEmitter, ::Components::CTransform>(
        [deltaTime](Entity /*entity*/, ::Components::CParticleEmitter& emitter, ::Components::CTransform& transform)
        {
            if (!emitter.isActive())
            {
                return;
            }

            Vec2 entityPos = transform.getPosition();
            Vec2 offset    = emitter.getPositionOffset();

            float rotation = transform.getRotation();
            if (rotation != 0.0f)
            {
                float cosR     = std::cos(rotation);
                float sinR     = std::sin(rotation);
                float rotatedX = offset.x * cosR - offset.y * sinR;
                float rotatedY = offset.x * sinR + offset.y * cosR;
                offset         = Vec2(rotatedX, rotatedY);
            }

            Vec2 worldPos = entityPos + offset;

            for (auto& particle : emitter.getParticles())
            {
                if (particle.alive)
                {
                    updateParticle(particle, &emitter, deltaTime);
                }
            }

            if (emitter.getEmissionRate() > 0.0f)
            {
                float timer            = emitter.getEmissionTimer() + deltaTime;
                float emissionInterval = 1.0f / emitter.getEmissionRate();

                while (timer >= emissionInterval)
                {
                    emitParticle(&emitter, worldPos, rotation);
                    timer -= emissionInterval;
                }
                emitter.setEmissionTimer(timer);
            }
        });
}

void SParticle::renderEmitter(Entity entity, sf::RenderWindow* window, World& world)
{
    static uint64_t s_renderEmitterFrameIndex = 0;

    sf::RenderWindow* targetWindow = window ? window : m_window;

    if (m_initialized == false || targetWindow == nullptr || !targetWindow->isOpen() || !entity.isValid())
    {
        return;
    }

    if (s_renderEmitterFrameIndex < 3)
    {
        LOG_INFO("Frame {}: SParticle::renderEmitter begin E{}:G{}", s_renderEmitterFrameIndex, entity.index, entity.generation);
    }

    auto* emitter = world.components().tryGet<::Components::CParticleEmitter>(entity);
    if (!emitter)
    {
        return;
    }

    if (s_renderEmitterFrameIndex < 3)
    {
        LOG_INFO("Frame {}: SParticle::renderEmitter got emitter (particles={}, alive={}, texturePath='{}')",
                 s_renderEmitterFrameIndex,
                 emitter->getParticles().size(),
                 emitter->getAliveCount(),
                 emitter->getTexturePath());
        LOG_INFO("Frame {}: SParticle::renderEmitter loadTexture begin", s_renderEmitterFrameIndex);
    }

    // Cache-only in the hot render path: if missing, queue it and draw fallback.
    const std::string& texturePath = emitter->getTexturePath();
    const sf::Texture* texture     = nullptr;
    if (!texturePath.empty())
    {
        const std::filesystem::path resolvedPath = Internal::ExecutablePaths::resolveRelativeToExecutableDir(texturePath);
        const std::string          resolvedStr  = resolvedPath.string();
        texture                                  = getCachedTexture(resolvedStr);
        if (!texture)
        {
            requestTextureLoad(texturePath);
        }
    }

    if (s_renderEmitterFrameIndex < 3)
    {
        LOG_INFO("Frame {}: SParticle::renderEmitter loadTexture end (texture={})",
                 s_renderEmitterFrameIndex,
                 texture ? "yes" : "no");
    }

    // Clear vertex array for this emitter
    m_vertexArray.clear();

    // Build vertex array for all alive particles
    for (const auto& particle : emitter->getParticles())
    {
        if (particle.alive == false)
        {
            continue;
        }

        if (s_renderEmitterFrameIndex < 3)
        {
            // Throttle: only log a small sample of alive particles
            static uint32_t s_loggedAliveParticles = 0;
            if (s_loggedAliveParticles < 8)
            {
                LOG_INFO("Frame {}: SParticle alive particle sample pos=({}, {}) size={} rot={} alpha={}",
                         s_renderEmitterFrameIndex,
                         particle.position.x,
                         particle.position.y,
                         particle.size,
                         particle.rotation,
                         particle.alpha);
                ++s_loggedAliveParticles;
            }
        }

        // Render in world space (meters). The active sf::View handles world->screen mapping.
        sf::Vector2f worldPos(static_cast<float>(particle.position.x), static_cast<float>(particle.position.y));
        float        halfSizeWorld = particle.size;

        // Create color with alpha
        const float clampedAlpha = std::clamp(particle.alpha, 0.0f, 1.0f);
        const auto  alphaByte    = static_cast<std::uint8_t>(clampedAlpha * 255.0f);
        // If a texture is missing/unavailable, fall back to a visible white quad.
        sf::Color   color = texture ? sf::Color(particle.color.r, particle.color.g, particle.color.b, alphaByte) : sf::Color(255, 255, 255, alphaByte);

        if (texture)
        {
            // Textured quad
            float cosR = std::cos(particle.rotation);
            float sinR = std::sin(particle.rotation);

            // Quad corners (centered)
            sf::Vector2f corners[4] = {
                sf::Vector2f(-halfSizeWorld, -halfSizeWorld),  // Top-left
                sf::Vector2f(halfSizeWorld, -halfSizeWorld),   // Top-right
                sf::Vector2f(halfSizeWorld, halfSizeWorld),    // Bottom-right
                sf::Vector2f(-halfSizeWorld, halfSizeWorld)    // Bottom-left
            };

            // Rotate corners
            for (int i = 0; i < 4; ++i)
            {
                float x      = corners[i].x;
                float y      = corners[i].y;
                corners[i].x = x * cosR - y * sinR;
                corners[i].y = x * sinR + y * cosR;
                corners[i] += worldPos;
            }

            // Texture coordinates (in pixels for SFML - confirmed by official docs)
            sf::Vector2u texSize      = texture->getSize();
            sf::Vector2f texCoords[4] = {
                sf::Vector2f(0.0f, 0.0f),                                                    // Top-left
                sf::Vector2f(static_cast<float>(texSize.x), 0.0f),                           // Top-right
                sf::Vector2f(static_cast<float>(texSize.x), static_cast<float>(texSize.y)),  // Bottom-right
                sf::Vector2f(0.0f, static_cast<float>(texSize.y))                            // Bottom-left
            };

            auto appendTextured = [&](int idx)
            {
                sf::Vertex v;
                v.position  = corners[idx];
                v.color     = color;
                v.texCoords = texCoords[idx];
                m_vertexArray.append(v);
            };

            // Two triangles: (0,1,2) and (0,2,3)
            appendTextured(0);
            appendTextured(1);
            appendTextured(2);
            appendTextured(0);
            appendTextured(2);
            appendTextured(3);
        }
        else
        {
            // Colored quad (no texture)
            float cosR = std::cos(particle.rotation);
            float sinR = std::sin(particle.rotation);

            sf::Vector2f corners[4] = {sf::Vector2f(-halfSizeWorld, -halfSizeWorld),
                                       sf::Vector2f(halfSizeWorld, -halfSizeWorld),
                                       sf::Vector2f(halfSizeWorld, halfSizeWorld),
                                       sf::Vector2f(-halfSizeWorld, halfSizeWorld)};

            for (int i = 0; i < 4; ++i)
            {
                float x      = corners[i].x;
                float y      = corners[i].y;
                corners[i].x = x * cosR - y * sinR;
                corners[i].y = x * sinR + y * cosR;
                corners[i] += worldPos;
            }

            auto appendColored = [&](int idx)
            {
                sf::Vertex v;
                v.position = corners[idx];
                v.color    = color;
                m_vertexArray.append(v);
            };

            // Two triangles: (0,1,2) and (0,2,3)
            appendColored(0);
            appendColored(1);
            appendColored(2);
            appendColored(0);
            appendColored(2);
            appendColored(3);
        }
    }

    // Render particles for this emitter
    if (m_vertexArray.getVertexCount() > 0)
    {
        sf::RenderStates states;
        states.blendMode = sf::BlendAlpha;

        if (texture)
        {
            states.texture = texture;
        }

        if (s_renderEmitterFrameIndex < 3)
        {
            LOG_INFO("Frame {}: SParticle::renderEmitter draw begin (verts={}, textured={})",
                     s_renderEmitterFrameIndex,
                     m_vertexArray.getVertexCount(),
                     texture ? "yes" : "no");
        }

        targetWindow->draw(m_vertexArray, states);

        if (s_renderEmitterFrameIndex < 3)
        {
            LOG_INFO("Frame {}: SParticle::renderEmitter draw end", s_renderEmitterFrameIndex);
        }
    }

    if (s_renderEmitterFrameIndex < 3)
    {
        LOG_INFO("Frame {}: SParticle::renderEmitter end E{}:G{}", s_renderEmitterFrameIndex, entity.index, entity.generation);
    }

    ++s_renderEmitterFrameIndex;
}

}  // namespace Systems
