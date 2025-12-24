#include "Barrel.h"

#include "BarrelBehaviour.h"

#include <Color.h>
#include <Components.h>
#include <World.h>

namespace Example
{

namespace
{
constexpr float kBarrelRadius              = 0.1f;
constexpr float kBarrelColliderDensity     = 0.5f;
constexpr float kBarrelColliderFriction    = 0.3f;
constexpr float kBarrelColliderRestitution = 0.0f;
constexpr float kBarrelLinearDamping       = 1.5f;
constexpr float kBarrelAngularDamping      = 2.0f;
constexpr float kBarrelGravityScale        = 1.0f;
constexpr int   kBarrelZIndex              = 10;
constexpr int   kBarrelSprayZIndex         = 9;

Components::CParticleEmitter makeBarrelSprayEmitter()
{
    Components::CParticleEmitter e;
    e.setDirection(Vec2(0.0f, 1.0f));
    e.setSpreadAngle(0.5f);
    e.setMinSpeed(0.05f);
    e.setMaxSpeed(0.75f);
    e.setMinLifetime(0.5f);
    e.setMaxLifetime(2.0f);
    e.setMinSize(0.006f);
    e.setMaxSize(0.02f);
    e.setEmissionRate(0.0f);
    e.setStartColor(Color(220, 240, 255, 255));
    e.setEndColor(Color(255, 255, 255, 255));
    e.setStartAlpha(0.9f);
    e.setEndAlpha(0.0f);
    e.setGravity(Vec2(0.0f, 0.0f));
    e.setMinRotationSpeed(-3.0f);
    e.setMaxRotationSpeed(3.0f);
    e.setFadeOut(true);
    e.setShrink(true);
    e.setShrinkEndScale(0.1f);
    e.setMaxParticles(1250);
    e.setZIndex(kBarrelSprayZIndex);
    e.setEmissionShape(Components::EmissionShape::Circle);
    e.setShapeRadius(kBarrelRadius);
    e.setEmitFromEdge(true);
    e.setEmitOutward(true);
    e.setLineStart(Vec2(-0.5f, 0.0f));
    e.setLineEnd(Vec2(0.5f, 0.0f));
    e.setTexturePath("assets/textures/bubble.png");
    return e;
}

}  // namespace

Entity spawnBarrel(World& world, const Vec2& position)
{
    Entity barrel = world.createEntity();

    world.components().add<Components::CName>(barrel, std::string("Barrel"));
    world.components().add<Components::CTransform>(barrel, position, Vec2{1.0f, 1.0f}, 0.0f);
    world.components().add<Components::CTexture>(barrel, "assets/textures/barrel.png");
    world.components().add<Components::CRenderable>(barrel, Components::VisualType::Sprite, Color::White, kBarrelZIndex, true);
    world.components().add<Components::CMaterial>(barrel, Color::White, Components::BlendMode::Alpha, 1.0f);

    Components::CPhysicsBody2D* body = world.components().add<Components::CPhysicsBody2D>(barrel);
    body->bodyType                   = Components::BodyType::Dynamic;
    body->fixedRotation              = false;
    body->linearDamping              = kBarrelLinearDamping;
    body->angularDamping             = kBarrelAngularDamping;
    body->gravityScale               = kBarrelGravityScale;

    Components::CCollider2D* collider = world.components().add<Components::CCollider2D>(barrel);
    collider->sensor                  = false;
    collider->density                 = kBarrelColliderDensity;
    collider->friction                = kBarrelColliderFriction;
    collider->restitution             = kBarrelColliderRestitution;
    collider->createCircle(kBarrelRadius);

    world.components().add<Components::CParticleEmitter>(barrel, makeBarrelSprayEmitter());

    Components::CNativeScript* script = world.components().add<Components::CNativeScript>(barrel);
    script->bind<BarrelBehaviour>();

    return barrel;
}

}  // namespace Example
