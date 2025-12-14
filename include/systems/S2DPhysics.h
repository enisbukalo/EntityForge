#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include "Entity.h"
#include "System.h"
#include "box2d/box2d.h"

namespace Components
{
class CPhysicsBody2D;
struct CTransform;
struct CCollider2D;
}  // namespace Components

namespace Systems
{

/**
 * @brief Box2D Physics System - Manages the Box2D physics world and simulation
 *
 * This system wraps Box2D v3.1.1 and provides:
 * - Physics world management
 * - Body creation and destruction
 * - Collision detection and response
 * - Physics simulation stepping
 * - Spatial queries (AABB, raycasting)
 *
 * Coordinate System: Y-up (positive Y = upward)
 * Units: 1 unit = 1 meter
 * Default Gravity: (0, -10) m/s²
 */
class S2DPhysics : public System
{
private:
    b2WorldId m_worldId;
    World*    m_world{nullptr};

    // System-owned Box2D bodies (runtime state)
    std::unordered_map<Entity, b2BodyId> m_bodies;

    // System-owned Box2D shapes/chains (runtime state)
    std::unordered_map<Entity, std::vector<b2ShapeId>> m_shapes;
    std::unordered_map<Entity, std::vector<b2ChainId>> m_chains;

    // Per-entity fixed-update callbacks
    std::unordered_map<Entity, std::function<void(float)>> m_fixedCallbacks;

    // Timestep settings
    float m_timeStep;
    int   m_subStepCount;

    // Internal helpers
    void ensureBodyForEntity(Entity entity, const Components::CTransform& transform, const Components::CPhysicsBody2D& body);
    void ensureShapesForEntity(Entity entity, const Components::CCollider2D& collider);
    void destroyShapes(Entity entity);
    void syncFromTransform(Entity entity, const Components::CTransform& transform);
    void syncToTransform(Entity entity, Components::CTransform& transform);
    void pruneDestroyedBodies(const World& world);
    void destroyBodyInternal(b2BodyId bodyId);

public:
    S2DPhysics();
    ~S2DPhysics();

    // Delete copy and move constructors/assignment operators
    S2DPhysics(const S2DPhysics&)            = delete;
    S2DPhysics(S2DPhysics&&)                 = delete;
    S2DPhysics& operator=(const S2DPhysics&) = delete;
    S2DPhysics& operator=(S2DPhysics&&)      = delete;

    /**
     * @brief Update the physics simulation
     * @param deltaTime Time elapsed since last update (not used - fixed timestep)
     */
    void update(float deltaTime, World& world) override;

    std::string_view name() const override
    {
        return "S2DPhysics";
    }

    bool usesFixedTimestep() const override
    {
        return true;
    }

    void fixedUpdate(float timeStep, World& world) override
    {
        (void)timeStep;
        runFixedUpdates(m_timeStep);
        update(m_timeStep, world);
    }

    /**
     * @brief Bind the world so physics can resolve components without side maps
     */
    void bindWorld(World* world)
    {
        m_world = world;
    }

    /**
     * @brief Get the Box2D world ID
     */
    b2WorldId getWorldId() const
    {
        return m_worldId;
    }

    /**
     * @brief Set world gravity
     * @param gravity Gravity vector in m/s² (e.g., {0, -10} for standard Earth gravity)
     */
    void setGravity(const b2Vec2& gravity);

    /**
     * @brief Get world gravity
     */
    b2Vec2 getGravity() const;

    /**
     * @brief Set the fixed timestep for physics simulation
     * @param timeStep Time step in seconds (default: 1/60)
     */
    void setTimeStep(float timeStep)
    {
        m_timeStep = timeStep;
    }

    /**
     * @brief Get the fixed timestep
     */
    float getTimeStep() const
    {
        return m_timeStep;
    }

    /**
     * @brief Set the number of sub-steps per physics update
     * @param subStepCount Number of sub-steps (default: 4)
     */
    void setSubStepCount(int subStepCount)
    {
        m_subStepCount = subStepCount;
    }

    /**
     * @brief Get the number of sub-steps
     */
    int getSubStepCount() const
    {
        return m_subStepCount;
    }

    /**
     * @brief Create a Box2D body for an entity
     * @param entity Entity ID to associate with the body
     * @param bodyDef Body definition
     * @return Box2D body ID
     */
    b2BodyId createBody(Entity entity, const b2BodyDef& bodyDef);

    /**
     * @brief Destroy the Box2D body associated with an entity
     * @param entity Entity ID whose body should be destroyed
     */
    void destroyBody(Entity entity);

    /**
     * @brief Destroy a Box2D body by handle (used by component cleanup)
     */
    void destroyBody(b2BodyId bodyId);

    /**
     * @brief Get the Box2D body associated with an entity
     * @param entity Entity ID to query
     * @return Body ID (invalid if entity has no body)
     */
    b2BodyId getBody(Entity entity) const;

    /**
     * @brief Query the world for all bodies overlapping an AABB
     * @param aabb Axis-aligned bounding box to query
     * @param callback Callback function called for each overlapping body
     */
    void queryAABB(const b2AABB& aabb, b2OverlapResultFcn* callback, void* context);

    /**
     * @brief Cast a ray through the world
     * @param origin Ray origin point
     * @param translation Ray direction and length
     * @param callback Callback function called for each hit
     */
    void rayCast(const b2Vec2& origin, const b2Vec2& translation, b2CastResultFcn* callback, void* context);

    /**
     * @brief Register a physics body for fixed-update callbacks
     * @param body Physics body component to register
     *
     * This is called automatically by CPhysicsBody2D::initialize().
     * Do not call manually unless you know what you're doing.
     */
    // Fixed update callbacks are now system-owned
    void setFixedUpdateCallback(Entity entity, std::function<void(float)> callback);
    void clearFixedUpdateCallback(Entity entity);

    // Body forces and queries
    void   applyForce(Entity entity, const b2Vec2& force, const b2Vec2& point);
    void   applyForceToCenter(Entity entity, const b2Vec2& force);
    void   applyLinearImpulse(Entity entity, const b2Vec2& impulse, const b2Vec2& point);
    void   applyLinearImpulseToCenter(Entity entity, const b2Vec2& impulse);
    void   applyAngularImpulse(Entity entity, float impulse);
    void   applyTorque(Entity entity, float torque);
    void   setLinearVelocity(Entity entity, const b2Vec2& velocity);
    void   setAngularVelocity(Entity entity, float omega);
    b2Vec2 getLinearVelocity(Entity entity) const;
    float  getAngularVelocity(Entity entity) const;
    b2Vec2 getPosition(Entity entity) const;
    float  getRotation(Entity entity) const;
    b2Vec2 getForwardVector(Entity entity) const;
    b2Vec2 getRightVector(Entity entity) const;

    void runFixedUpdates(float timeStep);
};

}  // namespace Systems
