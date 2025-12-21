#include "Boat.h"

#include "BoatBehaviour.h"

#include <cstddef>
#include <string>
#include <vector>

#include <Color.h>
#include <Components.h>
#include <World.h>

namespace Example
{

Entity spawnBoat(World& world)
{
    // Tuned values copied from the Example on `master`.
    static constexpr float kBoatPosX = 9.20209f;
    static constexpr float kBoatPosY = 7.90827f;
    static constexpr float kBoatRot  = 1.73084f;

    static constexpr float kBoatLinearDamping  = 0.75f;
    static constexpr float kBoatAngularDamping = 0.75f;
    static constexpr float kBoatGravityScale   = 1.0f;

    static constexpr float kBoatColliderDensity     = 5.0f;
    static constexpr float kBoatColliderFriction    = 0.5f;
    static constexpr float kBoatColliderRestitution = 0.125f;

    static const std::vector<std::vector<Vec2>> kBoatHullFixtures = {
        {{0.225f, 0.0f}, {-0.225f, 0.0f}, {-0.225f, -0.0875f}, {-0.1575f, -0.39375f}, {0.1575f, -0.39375f}, {0.225f, -0.0875f}},
        {{-0.225f, 0.0f}, {0.225f, 0.0f}, {0.223438f, 0.0401042f}, {-0.223438f, 0.0401042f}},
        {{-0.223438f, 0.0401042f}, {0.223438f, 0.0401042f}, {0.21875f, 0.0802083f}, {-0.21875f, 0.0802083f}},
        {{-0.21875f, 0.0802083f}, {0.21875f, 0.0802083f}, {0.210938f, 0.120313f}, {-0.210938f, 0.120313f}},
        {{-0.210938f, 0.120313f}, {0.210938f, 0.120313f}, {0.2f, 0.160417f}, {-0.2f, 0.160417f}},
        {{-0.2f, 0.160417f}, {0.2f, 0.160417f}, {0.185937f, 0.200521f}, {-0.185937f, 0.200521f}},
        {{-0.185937f, 0.200521f}, {0.185937f, 0.200521f}, {0.16875f, 0.240625f}, {-0.16875f, 0.240625f}},
        {{-0.16875f, 0.240625f}, {0.16875f, 0.240625f}, {0.148438f, 0.280729f}, {-0.148438f, 0.280729f}},
        {{-0.148438f, 0.280729f}, {0.148438f, 0.280729f}, {0.125f, 0.320833f}, {-0.125f, 0.320833f}},
        {{-0.125f, 0.320833f}, {0.125f, 0.320833f}, {0.0984375f, 0.360938f}, {-0.0984375f, 0.360938f}},
        {{-0.0984375f, 0.360938f}, {0.0984375f, 0.360938f}, {0.06875f, 0.401042f}, {-0.06875f, 0.401042f}},
        {{-0.06875f, 0.401042f}, {0.06875f, 0.401042f}, {0.0359375f, 0.441146f}, {-0.0359375f, 0.441146f}},
        {{-0.0359375f, 0.441146f}, {0.0359375f, 0.441146f}, {0.0f, 0.48125f}},
    };

    Entity boat = world.createEntity();

    world.components().add<Components::CTransform>(boat, Vec2{kBoatPosX, kBoatPosY}, Vec2{1.0f, 1.0f}, kBoatRot);
    world.components().add<Components::CTexture>(boat, "assets/textures/boat.png");
    world.components().add<Components::CRenderable>(boat, Components::VisualType::Sprite, Color::White, 10, true);
    world.components().add<Components::CMaterial>(boat, Color::White, Components::BlendMode::Alpha, 1.0f);

    Components::CPhysicsBody2D* body = world.components().add<Components::CPhysicsBody2D>(boat);
    body->bodyType                   = Components::BodyType::Dynamic;
    body->fixedRotation              = false;
    body->linearDamping              = kBoatLinearDamping;
    body->angularDamping             = kBoatAngularDamping;
    body->gravityScale               = kBoatGravityScale;

    Components::CCollider2D* collider = world.components().add<Components::CCollider2D>(boat);
    collider->sensor                  = false;
    collider->density                 = kBoatColliderDensity;
    collider->friction                = kBoatColliderFriction;
    collider->restitution             = kBoatColliderRestitution;
    if (!kBoatHullFixtures.empty())
    {
        collider->createPolygon(kBoatHullFixtures.front(), 0.02f);
        for (size_t i = 1; i < kBoatHullFixtures.size(); ++i)
        {
            collider->addPolygon(kBoatHullFixtures[i], 0.02f);
        }
    }

    world.components().add<Components::CInputController>(boat);

    // Spawn effect entities (particle emitters).
    // Behaviour will find them by CName and keep them pinned to the boat.
    {
        const auto* boatT = world.components().tryGet<Components::CTransform>(boat);
        const Vec2  pos   = boatT ? boatT->getPosition() : Vec2{0.0f, 0.0f};
        const float rot   = boatT ? boatT->getRotation() : 0.0f;

        // Bubble trail
        const Entity bubbleTrail = world.createEntity();
        world.components().add<Components::CName>(bubbleTrail, std::string("BoatBubbleTrail"));
        world.components().add<Components::CTransform>(bubbleTrail, pos, Vec2{1.0f, 1.0f}, rot);
        {
            Components::CParticleEmitter e;
            e.setDirection(Vec2(0.0f, -1.0f));
            e.setSpreadAngle(1.2f);
            e.setMinSpeed(0.05f);
            e.setMaxSpeed(0.2f);
            e.setMinLifetime(3.0f);
            e.setMaxLifetime(3.0f);
            e.setMinSize(0.005f);
            e.setMaxSize(0.025f);
            e.setEmissionRate(300.0f);
            e.setStartColor(Color::White);
            e.setEndColor(Color::White);
            e.setStartAlpha(1.0f);
            e.setEndAlpha(0.5f);
            e.setGravity(Vec2(0.0f, 0.0f));
            e.setFadeOut(true);
            e.setShrink(true);
            e.setShrinkEndScale(0.1f);
            e.setMaxParticles(1000);
            e.setZIndex(5);
            e.setPositionOffset(Vec2(0.0f, -0.65625f));
            e.setEmissionShape(Components::EmissionShape::Point);
            e.setLineStart(Vec2(-0.5f, 0.0f));
            e.setLineEnd(Vec2(0.5f, 0.0f));
            e.setEmitFromEdge(true);
            e.setEmitOutward(false);
            e.setTexturePath("assets/textures/bubble.png");
            world.components().add<Components::CParticleEmitter>(bubbleTrail, e);
        }

        // Hull spray
        const Entity hullSpray = world.createEntity();
        world.components().add<Components::CName>(hullSpray, std::string("BoatHullSpray"));
        world.components().add<Components::CTransform>(hullSpray, pos, Vec2{1.0f, 1.0f}, rot);
        {
            Components::CParticleEmitter e;
            e.setDirection(Vec2(0.0f, 1.0f));
            e.setSpreadAngle(0.4f);
            e.setMinSpeed(0.122925f);
            e.setMaxSpeed(0.4917f);
            e.setMinLifetime(0.5f);
            e.setMaxLifetime(2.4f);
            e.setMinSize(0.006f);
            e.setMaxSize(0.02f);
            e.setEmissionRate(938.808f);
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
            e.setMaxParticles(7500);
            e.setZIndex(5);
            e.setEmissionShape(Components::EmissionShape::Polygon);
            e.setEmitFromEdge(true);
            e.setEmitOutward(true);
            e.setLineStart(Vec2(-0.5f, 0.0f));
            e.setLineEnd(Vec2(0.5f, 0.0f));
            e.setTexturePath("assets/textures/bubble.png");
            e.setPolygonVertices({
                {-0.1575f, -0.39375f},    {0.1575f, -0.39375f},    {0.225f, -0.0875f},       {0.225f, 0.0f},
                {0.223438f, 0.0401042f},  {0.21875f, 0.0802083f},  {0.210938f, 0.120313f},   {0.2f, 0.160417f},
                {0.185937f, 0.200521f},   {0.16875f, 0.240625f},   {0.148438f, 0.280729f},   {0.125f, 0.320833f},
                {0.0984375f, 0.360938f},  {0.06875f, 0.401042f},   {0.0359375f, 0.441146f},  {0.0f, 0.48125f},
                {-0.0359375f, 0.441146f}, {-0.06875f, 0.401042f},  {-0.0984375f, 0.360938f}, {-0.125f, 0.320833f},
                {-0.148438f, 0.280729f},  {-0.16875f, 0.240625f},  {-0.185937f, 0.200521f},  {-0.2f, 0.160417f},
                {-0.210938f, 0.120313f},  {-0.21875f, 0.0802083f}, {-0.223438f, 0.0401042f}, {-0.225f, 0.0f},
                {-0.225f, -0.0875f},
            });
            world.components().add<Components::CParticleEmitter>(hullSpray, e);
        }
    }

    Components::CNativeScript* script = world.components().add<Components::CNativeScript>(boat);
    script->bind<Example::BoatBehaviour>();

    return boat;
}
}  // namespace Example
