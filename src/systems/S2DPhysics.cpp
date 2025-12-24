#include "S2DPhysics.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>
#include "CCollider2D.h"
#include "CPhysicsBody2D.h"
#include "CTransform.h"
#include "Logger.h"
#include "Vec2.h"
#include "World.h"

#include <physics/TriggerEvents.h>

namespace Systems
{

namespace
{
uint32_t floatBits(float v)
{
    uint32_t bits;
    static_assert(sizeof(bits) == sizeof(v));
    std::memcpy(&bits, &v, sizeof(v));
    return bits;
}

// cppcheck-suppress passedByValue
uint64_t fnv1a64(uint64_t hash, uint64_t data)
{
    constexpr uint64_t prime = 1099511628211ull;
    // cppcheck-suppress unreadVariable
    hash ^= data;
    // cppcheck-suppress uselessAssignmentArg
    // cppcheck-suppress unreadVariable
    hash *= prime;
    return hash;
}

// Incremental FNV-1a hashing: each call reads the previous hash value.
// cppcheck false-positives: fnv1a64(hash, x) reads hash before returning new value.
uint64_t hashCollider(const ::Components::CCollider2D& collider)
{
    uint64_t hash = 1469598103934665603ull;

    // cppcheck-suppress redundantAssignment
    hash = fnv1a64(hash, collider.sensor ? 1ull : 0ull);
    // cppcheck-suppress redundantAssignment
    hash = fnv1a64(hash, floatBits(collider.density));
    // cppcheck-suppress redundantAssignment
    hash = fnv1a64(hash, floatBits(collider.friction));
    // cppcheck-suppress redundantAssignment
    hash = fnv1a64(hash, floatBits(collider.restitution));
    // cppcheck-suppress redundantAssignment
    hash = fnv1a64(hash, static_cast<uint64_t>(collider.fixtures.size()));

    for (const auto& fixture : collider.fixtures)
    {
        // cppcheck-suppress redundantAssignment
        hash = fnv1a64(hash, static_cast<uint64_t>(fixture.shapeType));
        switch (fixture.shapeType)
        {
            case ::Components::ColliderShape::Circle:
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.circle.center.x));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.circle.center.y));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.circle.radius));
                break;
            case ::Components::ColliderShape::Box:
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.box.halfWidth));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.box.halfHeight));
                break;
            case ::Components::ColliderShape::Polygon:
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, static_cast<uint64_t>(fixture.polygon.vertices.size()));
                for (const auto& v : fixture.polygon.vertices)
                {
                    // cppcheck-suppress redundantAssignment
                    hash = fnv1a64(hash, floatBits(v.x));
                    // cppcheck-suppress redundantAssignment
                    hash = fnv1a64(hash, floatBits(v.y));
                }
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.polygon.radius));
                break;
            case ::Components::ColliderShape::Segment:
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.segment.point1.x));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.segment.point1.y));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.segment.point2.x));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.segment.point2.y));
                break;
            case ::Components::ColliderShape::ChainSegment:
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.chainSegment.ghost1.x));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.chainSegment.ghost1.y));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.chainSegment.point1.x));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.chainSegment.point1.y));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.chainSegment.point2.x));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.chainSegment.point2.y));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.chainSegment.ghost2.x));
                // cppcheck-suppress redundantAssignment
                hash = fnv1a64(hash, floatBits(fixture.chainSegment.ghost2.y));
                break;
        }
    }

    return hash;
}
}  // namespace

S2DPhysics::S2DPhysics() : m_timeStep(1.0f / 60.0f), m_subStepCount(6)
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity    = {0.0f, -10.0f};
    m_worldId           = b2CreateWorld(&worldDef);
    LOG_INFO("S2DPhysics: Initialized with timeStep={}, subStepCount={}", m_timeStep, m_subStepCount);
}

S2DPhysics::~S2DPhysics()
{
    LOG_INFO("S2DPhysics: Shutting down, destroying {} bodies", m_bodies.size());
    m_shapes.clear();
    m_chains.clear();
    m_bodies.clear();
    m_fixedCallbacks.clear();

    if (b2World_IsValid(m_worldId))
    {
        b2DestroyWorld(m_worldId);
    }
}

void S2DPhysics::update(float deltaTime, World& world)
{
    (void)deltaTime;

    m_world = &world;

    pruneDestroyedBodies(world);

    auto components = world.components();

    components.view2<::Components::CTransform, ::Components::CPhysicsBody2D>(
        [this](Entity entity, ::Components::CTransform& transform, ::Components::CPhysicsBody2D& body)
        {
            ensureBodyForEntity(entity, transform, body);

            auto* collider = m_world ? m_world->components().tryGet<::Components::CCollider2D>(entity) : nullptr;
            if (collider)
            {
                ensureShapesForEntity(entity, *collider);
            }
            else
            {
                destroyShapes(entity);
            }

            syncFromTransform(entity, transform, body);
        });

    b2World_Step(m_worldId, m_timeStep, m_subStepCount);

    // Drain Box2D sensor overlap events after stepping and republish via the engine EventBus.
    // EventBus delivery is staged (pumped by the engine), so these will be visible deterministically
    // the next time systems pump events.
    if (m_world)
    {
        const b2SensorEvents sensorEvents = b2World_GetSensorEvents(m_worldId);

        auto getEntityFromShapeId = [this](b2ShapeId shapeId) -> Entity
        {
            if (!b2Shape_IsValid(shapeId))
            {
                return Entity::null();
            }

            const b2BodyId bodyId = b2Shape_GetBody(shapeId);
            if (!b2Body_IsValid(bodyId))
            {
                return Entity::null();
            }

            const void* userData = b2Body_GetUserData(bodyId);
            if (userData == nullptr)
            {
                return Entity::null();
            }

            const std::uintptr_t packed = reinterpret_cast<std::uintptr_t>(userData);
            const auto           idx    = static_cast<std::uint32_t>(packed & 0xffffffffu);
            const auto           gen    = static_cast<std::uint32_t>((packed >> 32) & 0xffffffffu);
            const Entity         e{idx, gen};

            return (m_world && m_world->isAlive(e)) ? e : Entity::null();
        };

        for (int i = 0; i < sensorEvents.beginCount; ++i)
        {
            const b2SensorBeginTouchEvent& ev            = sensorEvents.beginEvents[i];
            const Entity                   triggerEntity = getEntityFromShapeId(ev.sensorShapeId);
            const Entity                   otherEntity   = getEntityFromShapeId(ev.visitorShapeId);

            if (triggerEntity.isValid() && otherEntity.isValid())
            {
                m_world->events().emit(Physics::TriggerEnter{triggerEntity, otherEntity});
            }
        }

        for (int i = 0; i < sensorEvents.endCount; ++i)
        {
            const b2SensorEndTouchEvent& ev            = sensorEvents.endEvents[i];
            const Entity                 triggerEntity = getEntityFromShapeId(ev.sensorShapeId);
            const Entity                 otherEntity   = getEntityFromShapeId(ev.visitorShapeId);

            if (triggerEntity.isValid() && otherEntity.isValid())
            {
                m_world->events().emit(Physics::TriggerExit{triggerEntity, otherEntity});
            }
        }
    }

    components.view2<::Components::CTransform, ::Components::CPhysicsBody2D>(
        [this](Entity entity, ::Components::CTransform& transform, ::Components::CPhysicsBody2D& body)
        {
            (void)body;
            syncToTransform(entity, transform);
        });
}

void S2DPhysics::setGravity(const b2Vec2& gravity)
{
    b2World_SetGravity(m_worldId, gravity);
}

b2Vec2 S2DPhysics::getGravity() const
{
    return b2World_GetGravity(m_worldId);
}

void S2DPhysics::destroyBodyInternal(b2BodyId bodyId)
{
    if (b2Body_IsValid(bodyId))
    {
        b2DestroyBody(bodyId);
    }
}

b2BodyId S2DPhysics::createBody(Entity entity, const b2BodyDef& bodyDef)
{
    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);
    if (entity.isValid() && b2Body_IsValid(bodyId))
    {
        // Store the full entity handle (index + generation) so contacts can be mapped reliably.
        const std::uintptr_t packed = (static_cast<std::uintptr_t>(entity.generation) << 32)
                                      | static_cast<std::uintptr_t>(entity.index);
        b2Body_SetUserData(bodyId, reinterpret_cast<void*>(packed));
    }
    return bodyId;
}

void S2DPhysics::destroyBody(Entity entity)
{
    if (!entity.isValid())
    {
        return;
    }

    destroyShapes(entity);

    auto it = m_bodies.find(entity);
    if (it != m_bodies.end())
    {
        destroyBody(it->second);
        m_bodies.erase(it);
    }

    m_fixedCallbacks.erase(entity);
}

void S2DPhysics::destroyShapes(Entity entity)
{
    auto shapesIt = m_shapes.find(entity);
    if (shapesIt != m_shapes.end())
    {
        for (b2ShapeId shapeId : shapesIt->second)
        {
            if (b2Shape_IsValid(shapeId))
            {
                b2DestroyShape(shapeId, true);
            }
        }
        m_shapes.erase(shapesIt);
    }

    auto chainsIt = m_chains.find(entity);
    if (chainsIt != m_chains.end())
    {
        for (b2ChainId chainId : chainsIt->second)
        {
            if (b2Chain_IsValid(chainId))
            {
                b2DestroyChain(chainId);
            }
        }
        m_chains.erase(chainsIt);
    }

    m_colliderHashes.erase(entity);
}

void S2DPhysics::ensureShapesForEntity(Entity entity, const ::Components::CCollider2D& collider)
{
    if (!entity.isValid())
    {
        return;
    }

    b2BodyId bodyId = getBody(entity);
    if (!b2Body_IsValid(bodyId))
    {
        destroyShapes(entity);
        return;
    }

    if (collider.fixtures.empty())
    {
        destroyShapes(entity);
        return;
    }

    const uint64_t currentHash = hashCollider(collider);
    const auto     hashIt      = m_colliderHashes.find(entity);
    const bool     hasShapes   = (m_shapes.find(entity) != m_shapes.end());
    if (hashIt != m_colliderHashes.end() && hashIt->second == currentHash && hasShapes)
    {
        return;
    }

    destroyShapes(entity);
    m_colliderHashes[entity] = currentHash;

    b2ShapeDef shapeDef           = b2DefaultShapeDef();
    shapeDef.density              = collider.density;
    shapeDef.material.friction    = collider.friction;
    shapeDef.material.restitution = collider.restitution;
    shapeDef.isSensor             = collider.sensor;
    // Box2D v3 requires explicit opt-in for sensor overlap events.
    // Enable for all shapes so sensors can report overlaps with any visitor.
    shapeDef.enableSensorEvents = true;

    auto& createdShapes = m_shapes[entity];

    for (const auto& fixture : collider.fixtures)
    {
        switch (fixture.shapeType)
        {
            case ::Components::ColliderShape::Circle:
            {
                b2Circle circle;
                circle.center = {fixture.circle.center.x, fixture.circle.center.y};
                circle.radius = fixture.circle.radius;

                b2ShapeId shapeId = b2CreateCircleShape(bodyId, &shapeDef, &circle);
                if (b2Shape_IsValid(shapeId))
                {
                    createdShapes.push_back(shapeId);
                }
                break;
            }
            case ::Components::ColliderShape::Box:
            {
                b2Polygon box     = b2MakeBox(fixture.box.halfWidth, fixture.box.halfHeight);
                b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &box);
                if (b2Shape_IsValid(shapeId))
                {
                    createdShapes.push_back(shapeId);
                }
                break;
            }
            case ::Components::ColliderShape::Polygon:
            {
                if (fixture.polygon.vertices.size() < 3)
                {
                    break;
                }

                std::vector<b2Vec2> points;
                points.reserve(fixture.polygon.vertices.size());
                std::transform(fixture.polygon.vertices.begin(),
                               fixture.polygon.vertices.end(),
                               std::back_inserter(points),
                               [](const Vec2& v) { return b2Vec2{v.x, v.y}; });

                b2Hull hull = b2ComputeHull(points.data(), static_cast<int>(points.size()));
                if (hull.count <= 0)
                {
                    break;
                }

                b2Polygon poly    = b2MakePolygon(&hull, fixture.polygon.radius);
                b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &poly);
                if (b2Shape_IsValid(shapeId))
                {
                    createdShapes.push_back(shapeId);
                }
                break;
            }
            case ::Components::ColliderShape::Segment:
            {
                b2Segment seg;
                seg.point1 = {fixture.segment.point1.x, fixture.segment.point1.y};
                seg.point2 = {fixture.segment.point2.x, fixture.segment.point2.y};

                b2ShapeId shapeId = b2CreateSegmentShape(bodyId, &shapeDef, &seg);
                if (b2Shape_IsValid(shapeId))
                {
                    createdShapes.push_back(shapeId);
                }
                break;
            }
            case ::Components::ColliderShape::ChainSegment:
            {
                // Represent a chain segment using a 4-point open chain:
                // ghost1 -> point1 -> point2 -> ghost2
                b2Vec2 pts[4] = {
                    {fixture.chainSegment.ghost1.x, fixture.chainSegment.ghost1.y},
                    {fixture.chainSegment.point1.x, fixture.chainSegment.point1.y},
                    {fixture.chainSegment.point2.x, fixture.chainSegment.point2.y},
                    {fixture.chainSegment.ghost2.x, fixture.chainSegment.ghost2.y},
                };

                b2SurfaceMaterial mat = b2DefaultSurfaceMaterial();
                mat.friction          = collider.friction;
                mat.restitution       = collider.restitution;

                b2ChainDef chainDef    = b2DefaultChainDef();
                chainDef.points        = pts;
                chainDef.count         = 4;
                chainDef.materials     = &mat;
                chainDef.materialCount = 1;
                // Enable sensor overlap events regardless of whether this chain is a sensor.
                chainDef.enableSensorEvents = true;
                chainDef.isLoop             = false;

                b2ChainId chainId = b2CreateChain(bodyId, &chainDef);
                if (b2Chain_IsValid(chainId))
                {
                    m_chains[entity].push_back(chainId);
                }
                break;
            }
        }
    }

    if (createdShapes.empty())
    {
        m_shapes.erase(entity);
    }
}

void S2DPhysics::destroyBody(b2BodyId bodyId)
{
    destroyBodyInternal(bodyId);
}

void S2DPhysics::ensureBodyForEntity(Entity entity, const ::Components::CTransform& transform, const ::Components::CPhysicsBody2D& body)
{
    if (!entity.isValid())
    {
        return;
    }

    b2BodyId existing = b2_nullBodyId;
    auto     it       = m_bodies.find(entity);
    if (it != m_bodies.end())
    {
        existing = it->second;
    }

    if (!b2Body_IsValid(existing))
    {
        b2BodyDef bodyDef      = b2DefaultBodyDef();
        bodyDef.position       = {transform.position.x, transform.position.y};
        bodyDef.linearDamping  = body.linearDamping;
        bodyDef.angularDamping = body.angularDamping;
        bodyDef.gravityScale   = body.gravityScale;

        switch (body.bodyType)
        {
            case Components::BodyType::Static:
                bodyDef.type = b2_staticBody;
                break;
            case Components::BodyType::Kinematic:
                bodyDef.type = b2_kinematicBody;
                break;
            case Components::BodyType::Dynamic:
                bodyDef.type = b2_dynamicBody;
                break;
        }

        existing = createBody(entity, bodyDef);
        if (b2Body_IsValid(existing))
        {
            m_bodies[entity] = existing;
        }
    }

    if (b2Body_IsValid(existing))
    {
        b2Body_SetFixedRotation(existing, body.fixedRotation);
        b2Body_SetLinearDamping(existing, body.linearDamping);
        b2Body_SetAngularDamping(existing, body.angularDamping);
        b2Body_SetGravityScale(existing, body.gravityScale);
    }
}

void S2DPhysics::pruneDestroyedBodies(const World& world)
{
    auto components = world.components();

    for (auto it = m_bodies.begin(); it != m_bodies.end();)
    {
        const Entity   entity = it->first;
        const b2BodyId bodyId = it->second;

        const bool deadEntity        = !world.isAlive(entity);
        const bool missingComponents = deadEntity || !components.has<::Components::CTransform>(entity)
                                       || !components.has<::Components::CPhysicsBody2D>(entity);
        const bool invalidBody = !b2Body_IsValid(bodyId);

        if (deadEntity || missingComponents || invalidBody)
        {
            destroyShapes(entity);
            if (b2Body_IsValid(bodyId))
            {
                destroyBody(bodyId);
            }
            m_fixedCallbacks.erase(entity);
            it = m_bodies.erase(it);
            continue;
        }

        // If a collider component was removed, destroy any attached shapes/chains.
        if (!components.has<::Components::CCollider2D>(entity))
        {
            destroyShapes(entity);
        }

        ++it;
    }
}

b2BodyId S2DPhysics::getBody(Entity entity) const
{
    if (!entity.isValid())
    {
        return b2_nullBodyId;
    }

    auto it = m_bodies.find(entity);
    return it != m_bodies.end() ? it->second : b2_nullBodyId;
}

void S2DPhysics::queryAABB(const b2AABB& aabb, b2OverlapResultFcn* callback, void* context)
{
    b2QueryFilter filter = b2DefaultQueryFilter();
    b2World_OverlapAABB(m_worldId, aabb, filter, callback, context);
}

void S2DPhysics::rayCast(const b2Vec2& origin, const b2Vec2& translation, b2CastResultFcn* callback, void* context)
{
    b2QueryFilter filter = b2DefaultQueryFilter();
    b2World_CastRay(m_worldId, origin, translation, filter, callback, context);
}

void S2DPhysics::setFixedUpdateCallback(Entity entity, std::function<void(float)> callback)
{
    if (!entity.isValid())
    {
        return;
    }

    if (!callback)
    {
        m_fixedCallbacks.erase(entity);
        return;
    }

    m_fixedCallbacks[entity] = std::move(callback);
}

void S2DPhysics::clearFixedUpdateCallback(Entity entity)
{
    m_fixedCallbacks.erase(entity);
}

void S2DPhysics::syncFromTransform(Entity entity, const ::Components::CTransform& transform, const ::Components::CPhysicsBody2D& body)
{
    const b2BodyId bodyId = getBody(entity);
    if (!b2Body_IsValid(bodyId))
    {
        return;
    }

    if (body.bodyType != ::Components::BodyType::Dynamic)
    {
        b2Body_SetTransform(bodyId, {transform.position.x, transform.position.y}, b2MakeRot(transform.rotation));
        b2Body_SetLinearVelocity(bodyId, {transform.velocity.x, transform.velocity.y});
        return;
    }

    const b2Vec2 bodyPos   = b2Body_GetPosition(bodyId);
    const float  bodyAngle = b2Rot_GetAngle(b2Body_GetRotation(bodyId));
    const b2Vec2 bodyVel   = b2Body_GetLinearVelocity(bodyId);

    const float dx        = transform.position.x - bodyPos.x;
    const float dy        = transform.position.y - bodyPos.y;
    const float posDiffSq = (dx * dx) + (dy * dy);

    const float dang    = transform.rotation - bodyAngle;
    const float dvelx   = transform.velocity.x - bodyVel.x;
    const float dvely   = transform.velocity.y - bodyVel.y;
    const float velDiff = (dvelx * dvelx) + (dvely * dvely);

    constexpr float kPosEpsSq = 1e-8f;
    constexpr float kAngleEps = 1e-4f;
    constexpr float kVelEpsSq = 1e-8f;
    if (posDiffSq > kPosEpsSq || std::fabs(dang) > kAngleEps || velDiff > kVelEpsSq)
    {
        b2Body_SetTransform(bodyId, {transform.position.x, transform.position.y}, b2MakeRot(transform.rotation));
        b2Body_SetLinearVelocity(bodyId, {transform.velocity.x, transform.velocity.y});
        // Ensure the body participates in broadphase/contact updates after being moved externally.
        b2Body_SetAwake(bodyId, true);
    }
}

void S2DPhysics::syncToTransform(Entity entity, ::Components::CTransform& transform)
{
    const b2BodyId bodyId = getBody(entity);
    if (!b2Body_IsValid(bodyId))
    {
        return;
    }

    b2Vec2 pos   = b2Body_GetPosition(bodyId);
    b2Rot  rot   = b2Body_GetRotation(bodyId);
    float  angle = b2Rot_GetAngle(rot);
    b2Vec2 vel   = b2Body_GetLinearVelocity(bodyId);

    transform.position = {pos.x, pos.y};
    transform.rotation = angle;
    transform.velocity = {vel.x, vel.y};
}

void S2DPhysics::applyForce(Entity entity, const b2Vec2& force, const b2Vec2& point)
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Body_ApplyForce(bodyId, force, point, true);
    }
}

void S2DPhysics::applyForceToCenter(Entity entity, const b2Vec2& force)
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Body_ApplyForceToCenter(bodyId, force, true);
    }
}

void S2DPhysics::applyLinearImpulse(Entity entity, const b2Vec2& impulse, const b2Vec2& point)
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Body_ApplyLinearImpulse(bodyId, impulse, point, true);
    }
}

void S2DPhysics::applyLinearImpulseToCenter(Entity entity, const b2Vec2& impulse)
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Body_ApplyLinearImpulseToCenter(bodyId, impulse, true);
    }
}

void S2DPhysics::applyAngularImpulse(Entity entity, float impulse)
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Body_ApplyAngularImpulse(bodyId, impulse, true);
    }
}

void S2DPhysics::applyTorque(Entity entity, float torque)
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Body_ApplyTorque(bodyId, torque, true);
    }
}

void S2DPhysics::setLinearVelocity(Entity entity, const b2Vec2& velocity)
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Body_SetLinearVelocity(bodyId, velocity);
    }
}

void S2DPhysics::setAngularVelocity(Entity entity, float omega)
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Body_SetAngularVelocity(bodyId, omega);
    }
}

b2Vec2 S2DPhysics::getLinearVelocity(Entity entity) const
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        return b2Body_GetLinearVelocity(bodyId);
    }
    return {0.0f, 0.0f};
}

float S2DPhysics::getAngularVelocity(Entity entity) const
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        return b2Body_GetAngularVelocity(bodyId);
    }
    return 0.0f;
}

b2Vec2 S2DPhysics::getPosition(Entity entity) const
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        return b2Body_GetPosition(bodyId);
    }
    return {0.0f, 0.0f};
}

float S2DPhysics::getRotation(Entity entity) const
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Rot rot = b2Body_GetRotation(bodyId);
        return b2Rot_GetAngle(rot);
    }
    return 0.0f;
}

b2Vec2 S2DPhysics::getForwardVector(Entity entity) const
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Rot rot = b2Body_GetRotation(bodyId);
        return b2Rot_GetYAxis(rot);
    }
    return {0.0f, 1.0f};
}

b2Vec2 S2DPhysics::getRightVector(Entity entity) const
{
    const b2BodyId bodyId = getBody(entity);
    if (b2Body_IsValid(bodyId))
    {
        b2Rot rot = b2Body_GetRotation(bodyId);
        return b2Rot_GetXAxis(rot);
    }
    return {1.0f, 0.0f};
}

void S2DPhysics::runFixedUpdates(float timeStep)
{
    for (auto it = m_fixedCallbacks.begin(); it != m_fixedCallbacks.end();)
    {
        const Entity   entity = it->first;
        const b2BodyId body   = getBody(entity);
        // The callback may be registered before the body is created (e.g., immediately after spawning an entity).
        // Keep the callback and start invoking it once the body exists.
        if (!b2Body_IsValid(body))
        {
            ++it;
            continue;
        }

        if (it->second)
        {
            it->second(timeStep);
        }
        ++it;
    }
}

}  // namespace Systems
