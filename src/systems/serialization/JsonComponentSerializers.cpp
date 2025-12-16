#include "JsonComponentSerializers.h"

#include <cstdint>
#include <string>
#include <typeindex>

#include <nlohmann/json.hpp>

#include "Logger.h"
#include "World.h"

#include "Components.h"

#include <ISerializableScript.h>

#include "ScriptTypeRegistry.h"

namespace
{

using json = nlohmann::json;

json vec2ToJson(const Vec2& v)
{
    return json { {"x", v.x}, {"y", v.y} };
}

Vec2 vec2FromJson(const json& j, const Vec2& fallback = Vec2 { 0.0f, 0.0f })
{
    if (!j.is_object())
    {
        return fallback;
    }
    return Vec2 { j.value("x", fallback.x), j.value("y", fallback.y) };
}

json colorToJson(const Color& c)
{
    return json { {"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a} };
}

Color colorFromJson(const json& j, const Color& fallback = Color {})
{
    if (!j.is_object())
    {
        return fallback;
    }

    auto toByte = [](int v)
    {
        if (v < 0)
        {
            v = 0;
        }
        if (v > 255)
        {
            v = 255;
        }
        return static_cast<uint8_t>(v);
    };

    return Color { toByte(j.value("r", static_cast<int>(fallback.r))),
                   toByte(j.value("g", static_cast<int>(fallback.g))),
                   toByte(j.value("b", static_cast<int>(fallback.b))),
                   toByte(j.value("a", static_cast<int>(fallback.a))) };
}

template <typename Enum>
Enum enumFromIntOrString(const json& j, Enum fallback, const std::unordered_map<std::string, Enum>& byName)
{
    if (j.is_string())
    {
        const std::string s = j.get<std::string>();
        auto              it = byName.find(s);
        if (it != byName.end())
        {
            return it->second;
        }
        return fallback;
    }
    if (j.is_number_integer())
    {
        return static_cast<Enum>(j.get<int>());
    }
    return fallback;
}

std::string visualTypeToString(Components::VisualType t)
{
    using VT = Components::VisualType;
    switch (t)
    {
        case VT::None: return "None";
        case VT::Rectangle: return "Rectangle";
        case VT::Circle: return "Circle";
        case VT::Sprite: return "Sprite";
        case VT::Line: return "Line";
        case VT::Custom: return "Custom";
    }
    return "None";
}

Components::VisualType visualTypeFromJson(const json& j)
{
    static const std::unordered_map<std::string, Components::VisualType> byName = {
        {"None", Components::VisualType::None},
        {"Rectangle", Components::VisualType::Rectangle},
        {"Circle", Components::VisualType::Circle},
        {"Sprite", Components::VisualType::Sprite},
        {"Line", Components::VisualType::Line},
        {"Custom", Components::VisualType::Custom},
    };
    return enumFromIntOrString(j, Components::VisualType::None, byName);
}

std::string blendModeToString(Components::BlendMode m)
{
    using BM = Components::BlendMode;
    switch (m)
    {
        case BM::Alpha: return "Alpha";
        case BM::Add: return "Add";
        case BM::Multiply: return "Multiply";
        case BM::None: return "None";
    }
    return "Alpha";
}

Components::BlendMode blendModeFromJson(const json& j)
{
    static const std::unordered_map<std::string, Components::BlendMode> byName = {
        {"Alpha", Components::BlendMode::Alpha},
        {"Add", Components::BlendMode::Add},
        {"Multiply", Components::BlendMode::Multiply},
        {"None", Components::BlendMode::None},
    };
    return enumFromIntOrString(j, Components::BlendMode::Alpha, byName);
}

std::string bodyTypeToString(Components::BodyType t)
{
    using BT = Components::BodyType;
    switch (t)
    {
        case BT::Static: return "Static";
        case BT::Kinematic: return "Kinematic";
        case BT::Dynamic: return "Dynamic";
    }
    return "Dynamic";
}

Components::BodyType bodyTypeFromJson(const json& j)
{
    static const std::unordered_map<std::string, Components::BodyType> byName = {
        {"Static", Components::BodyType::Static},
        {"Kinematic", Components::BodyType::Kinematic},
        {"Dynamic", Components::BodyType::Dynamic},
    };
    return enumFromIntOrString(j, Components::BodyType::Dynamic, byName);
}

std::string colliderShapeToString(Components::ColliderShape s)
{
    using CS = Components::ColliderShape;
    switch (s)
    {
        case CS::Circle: return "Circle";
        case CS::Box: return "Box";
        case CS::Polygon: return "Polygon";
        case CS::Segment: return "Segment";
        case CS::ChainSegment: return "ChainSegment";
    }
    return "Box";
}

Components::ColliderShape colliderShapeFromJson(const json& j)
{
    static const std::unordered_map<std::string, Components::ColliderShape> byName = {
        {"Circle", Components::ColliderShape::Circle},
        {"Box", Components::ColliderShape::Box},
        {"Polygon", Components::ColliderShape::Polygon},
        {"Segment", Components::ColliderShape::Segment},
        {"ChainSegment", Components::ColliderShape::ChainSegment},
    };
    return enumFromIntOrString(j, Components::ColliderShape::Box, byName);
}

std::string emissionShapeToString(Components::EmissionShape s)
{
    using ES = Components::EmissionShape;
    switch (s)
    {
        case ES::Point: return "Point";
        case ES::Circle: return "Circle";
        case ES::Rectangle: return "Rectangle";
        case ES::Line: return "Line";
        case ES::Polygon: return "Polygon";
    }
    return "Point";
}

Components::EmissionShape emissionShapeFromJson(const json& j)
{
    static const std::unordered_map<std::string, Components::EmissionShape> byName = {
        {"Point", Components::EmissionShape::Point},
        {"Circle", Components::EmissionShape::Circle},
        {"Rectangle", Components::EmissionShape::Rectangle},
        {"Line", Components::EmissionShape::Line},
        {"Polygon", Components::EmissionShape::Polygon},
    };
    return enumFromIntOrString(j, Components::EmissionShape::Point, byName);
}

std::string actionTriggerToString(ActionTrigger t)
{
    switch (t)
    {
        case ActionTrigger::Pressed: return "Pressed";
        case ActionTrigger::Held: return "Held";
        case ActionTrigger::Released: return "Released";
    }
    return "Pressed";
}

ActionTrigger actionTriggerFromJson(const json& j)
{
    static const std::unordered_map<std::string, ActionTrigger> byName = {
        {"Pressed", ActionTrigger::Pressed},
        {"Held", ActionTrigger::Held},
        {"Released", ActionTrigger::Released},
    };
    return enumFromIntOrString(j, ActionTrigger::Pressed, byName);
}

json entityRefToJson(Entity e, const Serialization::SaveContext& ctx)
{
    if (!e.isValid() || ctx.entityToSavedId == nullptr)
    {
        return nullptr;
    }

    auto it = ctx.entityToSavedId->find(e);
    if (it == ctx.entityToSavedId->end())
    {
        return nullptr;
    }
    return it->second;
}

Entity entityRefFromJson(const json& j, const Serialization::LoadContext& ctx)
{
    if (!j.is_string() || ctx.savedIdToEntity == nullptr)
    {
        return Entity::null();
    }

    auto it = ctx.savedIdToEntity->find(j.get<std::string>());
    if (it == ctx.savedIdToEntity->end())
    {
        return Entity::null();
    }
    return it->second;
}

}  // namespace

namespace Serialization
{

void registerBuiltInJsonComponentSerializers(ComponentSerializationRegistry& registry)
{
    using json = nlohmann::json;

    // CTransform
    registry.registerComponent(
        "CTransform",
        [](const World& w, Entity e) { return w.has<Components::CTransform>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CTransform>(e);
            return json {
                {"position", vec2ToJson(c->position)},
                {"velocity", vec2ToJson(c->velocity)},
                {"scale", vec2ToJson(c->scale)},
                {"rotation", c->rotation},
            };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CTransform t;
            t.position = vec2FromJson(data.value("position", json {}), t.position);
            t.velocity = vec2FromJson(data.value("velocity", json {}), t.velocity);
            t.scale    = vec2FromJson(data.value("scale", json {}), t.scale);
            t.rotation = data.value("rotation", t.rotation);
            w.add<Components::CTransform>(e, t);
        });

    // CRenderable
    registry.registerComponent(
        "CRenderable",
        [](const World& w, Entity e) { return w.has<Components::CRenderable>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CRenderable>(e);
            return json {
                {"visualType", visualTypeToString(c->visualType)},
                {"color", colorToJson(c->color)},
                {"zIndex", c->zIndex},
                {"visible", c->visible},
                {"lineStart", vec2ToJson(c->lineStart)},
                {"lineEnd", vec2ToJson(c->lineEnd)},
                {"lineThickness", c->lineThickness},
            };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CRenderable r;
            r.visualType     = visualTypeFromJson(data.value("visualType", json {}));
            r.color          = colorFromJson(data.value("color", json {}), r.color);
            r.zIndex         = data.value("zIndex", r.zIndex);
            r.visible        = data.value("visible", r.visible);
            r.lineStart      = vec2FromJson(data.value("lineStart", json {}), r.lineStart);
            r.lineEnd        = vec2FromJson(data.value("lineEnd", json {}), r.lineEnd);
            r.lineThickness  = data.value("lineThickness", r.lineThickness);
            w.add<Components::CRenderable>(e, r);
        });

    // CName
    registry.registerComponent(
        "CName",
        [](const World& w, Entity e) { return w.has<Components::CName>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CName>(e);
            return json { {"name", c->name} };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CName n;
            n.name = data.value("name", n.name);
            w.add<Components::CName>(e, n);
        });

    // CTexture
    registry.registerComponent(
        "CTexture",
        [](const World& w, Entity e) { return w.has<Components::CTexture>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CTexture>(e);
            return json { {"texturePath", c->texturePath} };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CTexture t;
            t.texturePath = data.value("texturePath", t.texturePath);
            w.add<Components::CTexture>(e, t);
        });

    // CShader
    registry.registerComponent(
        "CShader",
        [](const World& w, Entity e) { return w.has<Components::CShader>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CShader>(e);
            return json { {"vertexShaderPath", c->vertexShaderPath}, {"fragmentShaderPath", c->fragmentShaderPath} };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CShader s;
            s.vertexShaderPath   = data.value("vertexShaderPath", s.vertexShaderPath);
            s.fragmentShaderPath = data.value("fragmentShaderPath", s.fragmentShaderPath);
            w.add<Components::CShader>(e, s);
        });

    // CMaterial
    registry.registerComponent(
        "CMaterial",
        [](const World& w, Entity e) { return w.has<Components::CMaterial>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CMaterial>(e);
            return json {
                {"tint", colorToJson(c->tint)},
                {"blendMode", blendModeToString(c->blendMode)},
                {"opacity", c->opacity},
            };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CMaterial m;
            m.tint      = colorFromJson(data.value("tint", json {}), m.tint);
            m.blendMode = blendModeFromJson(data.value("blendMode", json {}));
            m.opacity   = data.value("opacity", m.opacity);
            w.add<Components::CMaterial>(e, m);
        });

    // CPhysicsBody2D
    registry.registerComponent(
        "CPhysicsBody2D",
        [](const World& w, Entity e) { return w.has<Components::CPhysicsBody2D>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CPhysicsBody2D>(e);
            return json {
                {"bodyType", bodyTypeToString(c->bodyType)},
                {"density", c->density},
                {"friction", c->friction},
                {"restitution", c->restitution},
                {"fixedRotation", c->fixedRotation},
                {"linearDamping", c->linearDamping},
                {"angularDamping", c->angularDamping},
                {"gravityScale", c->gravityScale},
            };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CPhysicsBody2D b;
            b.bodyType       = bodyTypeFromJson(data.value("bodyType", json {}));
            b.density        = data.value("density", b.density);
            b.friction       = data.value("friction", b.friction);
            b.restitution    = data.value("restitution", b.restitution);
            b.fixedRotation  = data.value("fixedRotation", b.fixedRotation);
            b.linearDamping  = data.value("linearDamping", b.linearDamping);
            b.angularDamping = data.value("angularDamping", b.angularDamping);
            b.gravityScale   = data.value("gravityScale", b.gravityScale);
            w.add<Components::CPhysicsBody2D>(e, b);
        });

    // CCollider2D
    registry.registerComponent(
        "CCollider2D",
        [](const World& w, Entity e) { return w.has<Components::CCollider2D>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CCollider2D>(e);

            json fixtures = json::array();
            for (const auto& f : c->fixtures)
            {
                json fj;
                fj["shapeType"] = colliderShapeToString(f.shapeType);
                fj["circle"] = { {"center", vec2ToJson(f.circle.center)}, {"radius", f.circle.radius} };
                fj["box"] = { {"halfWidth", f.box.halfWidth}, {"halfHeight", f.box.halfHeight} };

                json polyVerts = json::array();
                for (const auto& v : f.polygon.vertices)
                {
                    polyVerts.push_back(vec2ToJson(v));
                }
                fj["polygon"] = { {"vertices", std::move(polyVerts)}, {"radius", f.polygon.radius} };

                fj["segment"] = { {"point1", vec2ToJson(f.segment.point1)}, {"point2", vec2ToJson(f.segment.point2)} };
                fj["chainSegment"] = {
                    {"ghost1", vec2ToJson(f.chainSegment.ghost1)},
                    {"point1", vec2ToJson(f.chainSegment.point1)},
                    {"point2", vec2ToJson(f.chainSegment.point2)},
                    {"ghost2", vec2ToJson(f.chainSegment.ghost2)},
                };
                fixtures.push_back(std::move(fj));
            }

            return json {
                {"sensor", c->sensor},
                {"density", c->density},
                {"friction", c->friction},
                {"restitution", c->restitution},
                {"fixtures", std::move(fixtures)},
            };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CCollider2D c;
            c.sensor      = data.value("sensor", c.sensor);
            c.density     = data.value("density", c.density);
            c.friction    = data.value("friction", c.friction);
            c.restitution = data.value("restitution", c.restitution);

            if (data.contains("fixtures") && data["fixtures"].is_array())
            {
                for (const auto& fj : data["fixtures"])
                {
                    Components::ShapeFixture f;
                    f.shapeType = colliderShapeFromJson(fj.value("shapeType", json {}));

                    if (fj.contains("circle"))
                    {
                        const auto& cj = fj["circle"];
                        f.circle.center = vec2FromJson(cj.value("center", json {}), f.circle.center);
                        f.circle.radius = cj.value("radius", f.circle.radius);
                    }
                    if (fj.contains("box"))
                    {
                        const auto& bj = fj["box"];
                        f.box.halfWidth  = bj.value("halfWidth", f.box.halfWidth);
                        f.box.halfHeight = bj.value("halfHeight", f.box.halfHeight);
                    }
                    if (fj.contains("polygon"))
                    {
                        const auto& pj = fj["polygon"];
                        f.polygon.radius = pj.value("radius", f.polygon.radius);
                        if (pj.contains("vertices") && pj["vertices"].is_array())
                        {
                            f.polygon.vertices.clear();
                            for (const auto& vj : pj["vertices"])
                            {
                                f.polygon.vertices.push_back(vec2FromJson(vj));
                            }
                        }
                    }
                    if (fj.contains("segment"))
                    {
                        const auto& sj = fj["segment"];
                        f.segment.point1 = vec2FromJson(sj.value("point1", json {}), f.segment.point1);
                        f.segment.point2 = vec2FromJson(sj.value("point2", json {}), f.segment.point2);
                    }
                    if (fj.contains("chainSegment"))
                    {
                        const auto& cj = fj["chainSegment"];
                        f.chainSegment.ghost1 = vec2FromJson(cj.value("ghost1", json {}), f.chainSegment.ghost1);
                        f.chainSegment.point1 = vec2FromJson(cj.value("point1", json {}), f.chainSegment.point1);
                        f.chainSegment.point2 = vec2FromJson(cj.value("point2", json {}), f.chainSegment.point2);
                        f.chainSegment.ghost2 = vec2FromJson(cj.value("ghost2", json {}), f.chainSegment.ghost2);
                    }

                    c.fixtures.push_back(std::move(f));
                }
            }

            w.add<Components::CCollider2D>(e, c);
        });

    // CInputController (bindings only; runtime actionStates are not persisted)
    registry.registerComponent(
        "CInputController",
        [](const World& w, Entity e) { return w.has<Components::CInputController>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CInputController>(e);
            json        bindingsObj = json::object();

            for (const auto& [action, vec] : c->bindings)
            {
                json arr = json::array();
                for (const auto& b : vec)
                {
                    json keys = json::array();
                    for (KeyCode k : b.binding.keys)
                    {
                        keys.push_back(static_cast<int>(k));
                    }
                    json mouse = json::array();
                    for (MouseButton mb : b.binding.mouseButtons)
                    {
                        mouse.push_back(static_cast<int>(mb));
                    }
                    arr.push_back(json {
                        {"keys", std::move(keys)},
                        {"mouseButtons", std::move(mouse)},
                        {"trigger", actionTriggerToString(b.binding.trigger)},
                        {"allowRepeat", b.binding.allowRepeat},
                    });
                }
                bindingsObj[action] = std::move(arr);
            }

            return json { {"bindings", std::move(bindingsObj)} };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CInputController c;
            if (data.contains("bindings") && data["bindings"].is_object())
            {
                for (auto it = data["bindings"].begin(); it != data["bindings"].end(); ++it)
                {
                    const std::string action = it.key();
                    const auto&       arr    = it.value();
                    if (!arr.is_array())
                    {
                        continue;
                    }

                    std::vector<Components::CInputController::Binding> out;
                    out.reserve(arr.size());

                    for (const auto& bj : arr)
                    {
                        Components::CInputController::Binding b;
                        b.bindingId = 0;

                        if (bj.contains("keys") && bj["keys"].is_array())
                        {
                            for (const auto& kj : bj["keys"])
                            {
                                if (kj.is_number_integer())
                                {
                                    b.binding.keys.push_back(static_cast<KeyCode>(kj.get<int>()));
                                }
                            }
                        }

                        if (bj.contains("mouseButtons") && bj["mouseButtons"].is_array())
                        {
                            for (const auto& mj : bj["mouseButtons"])
                            {
                                if (mj.is_number_integer())
                                {
                                    b.binding.mouseButtons.push_back(static_cast<MouseButton>(mj.get<int>()));
                                }
                            }
                        }

                        b.binding.trigger     = actionTriggerFromJson(bj.value("trigger", json {}));
                        b.binding.allowRepeat = bj.value("allowRepeat", b.binding.allowRepeat);

                        out.push_back(std::move(b));
                    }
                    c.bindings.emplace(action, std::move(out));
                }
            }

            w.add<Components::CInputController>(e, c);
        });

    // CAudioSource (persist configuration only; runtime play/stop requests are not persisted)
    registry.registerComponent(
        "CAudioSource",
        [](const World& w, Entity e) { return w.has<Components::CAudioSource>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CAudioSource>(e);
            return json { {"clipId", c->clipId}, {"volume", c->volume}, {"loop", c->loop} };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CAudioSource a;
            a.clipId = data.value("clipId", a.clipId);
            a.volume = data.value("volume", a.volume);
            a.loop   = data.value("loop", a.loop);
            a.playRequested = false;
            a.stopRequested = false;
            w.add<Components::CAudioSource>(e, a);
        });

    // CAudioListener (persist volumes)
    registry.registerComponent(
        "CAudioListener",
        [](const World& w, Entity e) { return w.has<Components::CAudioListener>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CAudioListener>(e);
            return json { {"masterVolume", c->masterVolume}, {"musicVolume", c->musicVolume} };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CAudioListener l;
            l.masterVolume = data.value("masterVolume", l.masterVolume);
            l.musicVolume  = data.value("musicVolume", l.musicVolume);
            w.add<Components::CAudioListener>(e, l);
        });

    // CAudioSettings (persistent audio configuration)
    registry.registerComponent(
        "CAudioSettings",
        [](const World& w, Entity e) { return w.has<Components::CAudioSettings>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CAudioSettings>(e);
            return json {
                {"masterVolume", c->masterVolume},
                {"musicVolume", c->musicVolume},
                {"sfxVolume", c->sfxVolume},
            };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CAudioSettings s;
            s.masterVolume = data.value("masterVolume", s.masterVolume);
            s.musicVolume  = data.value("musicVolume", s.musicVolume);
            s.sfxVolume    = data.value("sfxVolume", s.sfxVolume);
            w.add<Components::CAudioSettings>(e, s);
        });

    // CCamera
    registry.registerComponent(
        "CCamera",
        [](const World& w, Entity e) { return w.has<Components::CCamera>(e); },
        [](const World& w, Entity e, const SaveContext& ctx) -> json
        {
            const auto* c = w.get<Components::CCamera>(e);
            json        viewport = {
                {"left", c->viewport.left},
                {"top", c->viewport.top},
                {"width", c->viewport.width},
                {"height", c->viewport.height},
            };
            json clampRect = {
                {"min", vec2ToJson(c->clampRect.min)},
                {"max", vec2ToJson(c->clampRect.max)},
            };

            return json {
                {"name", c->name},
                {"enabled", c->enabled},
                {"render", c->render},
                {"renderOrder", c->renderOrder},
                {"followTarget", entityRefToJson(c->followTarget, ctx)},
                {"followEnabled", c->followEnabled},
                {"followOffset", vec2ToJson(c->followOffset)},
                {"zoom", c->zoom},
                {"rotationRadians", c->rotationRadians},
                {"worldHeight", c->worldHeight},
                {"position", vec2ToJson(c->position)},
                {"viewport", std::move(viewport)},
                {"clampEnabled", c->clampEnabled},
                {"clampRect", std::move(clampRect)},
            };
        },
        [](World& w, Entity e, const json& data, const LoadContext& ctx)
        {
            Components::CCamera c;
            c.name        = data.value("name", c.name);
            c.enabled     = data.value("enabled", c.enabled);
            c.render      = data.value("render", c.render);
            c.renderOrder = data.value("renderOrder", c.renderOrder);

            c.followTarget  = entityRefFromJson(data.value("followTarget", json {}), ctx);
            c.followEnabled = data.value("followEnabled", c.followEnabled);
            c.followOffset  = vec2FromJson(data.value("followOffset", json {}), c.followOffset);

            c.zoom            = data.value("zoom", c.zoom);
            c.rotationRadians = data.value("rotationRadians", c.rotationRadians);
            c.worldHeight     = data.value("worldHeight", c.worldHeight);
            c.position        = vec2FromJson(data.value("position", json {}), c.position);

            if (data.contains("viewport") && data["viewport"].is_object())
            {
                const auto& v = data["viewport"];
                c.viewport.left   = v.value("left", c.viewport.left);
                c.viewport.top    = v.value("top", c.viewport.top);
                c.viewport.width  = v.value("width", c.viewport.width);
                c.viewport.height = v.value("height", c.viewport.height);
            }

            c.clampEnabled = data.value("clampEnabled", c.clampEnabled);
            if (data.contains("clampRect") && data["clampRect"].is_object())
            {
                const auto& cr = data["clampRect"];
                c.clampRect.min = vec2FromJson(cr.value("min", json {}), c.clampRect.min);
                c.clampRect.max = vec2FromJson(cr.value("max", json {}), c.clampRect.max);
            }

            w.add<Components::CCamera>(e, c);
        });

    // CParticleEmitter (persist configuration only; runtime particle list is not persisted)
    registry.registerComponent(
        "CParticleEmitter",
        [](const World& w, Entity e) { return w.has<Components::CParticleEmitter>(e); },
        [](const World& w, Entity e, const SaveContext&) -> json
        {
            const auto* c = w.get<Components::CParticleEmitter>(e);
            json        polygon = json::array();
            for (const auto& v : c->getPolygonVertices())
            {
                polygon.push_back(vec2ToJson(v));
            }

            return json {
                {"active", c->isActive()},
                {"direction", vec2ToJson(c->getDirection())},
                {"spreadAngle", c->getSpreadAngle()},
                {"minSpeed", c->getMinSpeed()},
                {"maxSpeed", c->getMaxSpeed()},
                {"minLifetime", c->getMinLifetime()},
                {"maxLifetime", c->getMaxLifetime()},
                {"minSize", c->getMinSize()},
                {"maxSize", c->getMaxSize()},
                {"emissionRate", c->getEmissionRate()},
                {"burstCount", c->getBurstCount()},
                {"startColor", colorToJson(c->getStartColor())},
                {"endColor", colorToJson(c->getEndColor())},
                {"startAlpha", c->getStartAlpha()},
                {"endAlpha", c->getEndAlpha()},
                {"gravity", vec2ToJson(c->getGravity())},
                {"minRotationSpeed", c->getMinRotationSpeed()},
                {"maxRotationSpeed", c->getMaxRotationSpeed()},
                {"fadeOut", c->getFadeOut()},
                {"shrink", c->getShrink()},
                {"shrinkEndScale", c->getShrinkEndScale()},
                {"maxParticles", c->getMaxParticles()},
                {"positionOffset", vec2ToJson(c->getPositionOffset())},
                {"emissionShape", emissionShapeToString(c->getEmissionShape())},
                {"shapeRadius", c->getShapeRadius()},
                {"shapeSize", vec2ToJson(c->getShapeSize())},
                {"lineStart", vec2ToJson(c->getLineStart())},
                {"lineEnd", vec2ToJson(c->getLineEnd())},
                {"emitFromEdge", c->getEmitFromEdge()},
                {"emitOutward", c->getEmitOutward()},
                {"polygonVertices", std::move(polygon)},
                {"texturePath", c->getTexturePath()},
                {"zIndex", c->getZIndex()},
            };
        },
        [](World& w, Entity e, const json& data, const LoadContext&)
        {
            Components::CParticleEmitter p;
            p.setActive(data.value("active", true));
            p.setDirection(vec2FromJson(data.value("direction", json {}), p.getDirection()));
            p.setSpreadAngle(data.value("spreadAngle", p.getSpreadAngle()));
            p.setMinSpeed(data.value("minSpeed", p.getMinSpeed()));
            p.setMaxSpeed(data.value("maxSpeed", p.getMaxSpeed()));
            p.setMinLifetime(data.value("minLifetime", p.getMinLifetime()));
            p.setMaxLifetime(data.value("maxLifetime", p.getMaxLifetime()));
            p.setMinSize(data.value("minSize", p.getMinSize()));
            p.setMaxSize(data.value("maxSize", p.getMaxSize()));
            p.setEmissionRate(data.value("emissionRate", p.getEmissionRate()));
            p.setBurstCount(data.value("burstCount", p.getBurstCount()));
            p.setStartColor(colorFromJson(data.value("startColor", json {}), p.getStartColor()));
            p.setEndColor(colorFromJson(data.value("endColor", json {}), p.getEndColor()));
            p.setStartAlpha(data.value("startAlpha", p.getStartAlpha()));
            p.setEndAlpha(data.value("endAlpha", p.getEndAlpha()));
            p.setGravity(vec2FromJson(data.value("gravity", json {}), p.getGravity()));
            p.setMinRotationSpeed(data.value("minRotationSpeed", p.getMinRotationSpeed()));
            p.setMaxRotationSpeed(data.value("maxRotationSpeed", p.getMaxRotationSpeed()));
            p.setFadeOut(data.value("fadeOut", p.getFadeOut()));
            p.setShrink(data.value("shrink", p.getShrink()));
            p.setShrinkEndScale(data.value("shrinkEndScale", p.getShrinkEndScale()));
            p.setMaxParticles(data.value("maxParticles", p.getMaxParticles()));
            p.setPositionOffset(vec2FromJson(data.value("positionOffset", json {}), p.getPositionOffset()));

            p.setEmissionShape(emissionShapeFromJson(data.value("emissionShape", json {})));
            p.setShapeRadius(data.value("shapeRadius", p.getShapeRadius()));
            p.setShapeSize(vec2FromJson(data.value("shapeSize", json {}), p.getShapeSize()));
            p.setLineStart(vec2FromJson(data.value("lineStart", json {}), p.getLineStart()));
            p.setLineEnd(vec2FromJson(data.value("lineEnd", json {}), p.getLineEnd()));
            p.setEmitFromEdge(data.value("emitFromEdge", p.getEmitFromEdge()));
            p.setEmitOutward(data.value("emitOutward", p.getEmitOutward()));

            if (data.contains("polygonVertices") && data["polygonVertices"].is_array())
            {
                std::vector<Vec2> verts;
                for (const auto& vj : data["polygonVertices"])
                {
                    verts.push_back(vec2FromJson(vj));
                }
                p.setPolygonVertices(verts);
            }

            p.setTexturePath(data.value("texturePath", p.getTexturePath()));
            p.setZIndex(data.value("zIndex", p.getZIndex()));

            // Runtime state intentionally reset.
            p.getParticles().clear();
            p.setEmissionTimer(0.0f);

            w.add<Components::CParticleEmitter>(e, p);
        });

    // CNativeScript: save script type name + optional script-defined fields.
    registry.registerComponent(
        "CNativeScript",
        [](const World& w, Entity e) { return w.has<Components::CNativeScript>(e); },
        [](const World& w, Entity e, const SaveContext& /*ctx*/) -> json
        {
            const auto* c = w.get<Components::CNativeScript>(e);

            std::string scriptType;
            if (c)
            {
                scriptType = c->scriptTypeName;
                if (scriptType.empty() && c->instance)
                {
                    scriptType = ScriptTypeRegistry::instance().stableNameFor(std::type_index(typeid(*c->instance)));
                }
            }

            json fields = json::object();
            if (c && c->instance)
            {
                if (const auto* serializable = dynamic_cast<const Components::ISerializableScript*>(c->instance.get()))
                {
                    Serialization::ScriptFieldWriter writer;
                    serializable->serializeFields(writer);

                    for (const auto& [key, value] : writer.fields())
                    {
                        if (const auto* i = std::get_if<std::int64_t>(&value))
                        {
                            fields[key] = *i;
                        }
                        else if (const auto* f = std::get_if<double>(&value))
                        {
                            fields[key] = *f;
                        }
                        else if (const auto* b = std::get_if<bool>(&value))
                        {
                            fields[key] = *b;
                        }
                        else if (const auto* s = std::get_if<std::string>(&value))
                        {
                            fields[key] = *s;
                        }
                    }
                }
            }

            return json { {"scriptType", scriptType}, {"fields", std::move(fields)} };
        },
        [](World& w, Entity e, const json& data, const LoadContext& /*ctx*/)
        {
            const std::string scriptType = data.value("scriptType", std::string {});

            std::unique_ptr<Components::INativeScript> instance;
            if (!scriptType.empty())
            {
                instance = ScriptTypeRegistry::instance().create(scriptType);
                if (!instance)
                {
                    LOG_WARN("SaveGame: unknown script type '{}' in save; leaving CNativeScript unbound", scriptType);
                }
            }

            auto* c = w.components().add<Components::CNativeScript>(e);
            c->instance        = std::move(instance);
            c->scriptTypeName  = scriptType;
            c->created         = false;

            if (c->instance && data.contains("fields") && data["fields"].is_object())
            {
                if (auto* serializable = dynamic_cast<Components::ISerializableScript*>(c->instance.get()))
                {
                    std::unordered_map<std::string, Serialization::ScriptFieldValue> fields;
                    for (auto it = data["fields"].begin(); it != data["fields"].end(); ++it)
                    {
                        if (!it.key().empty())
                        {
                            const auto& v = it.value();
                            if (v.is_boolean())
                            {
                                fields.emplace(it.key(), v.get<bool>());
                            }
                            else if (v.is_number_integer())
                            {
                                fields.emplace(it.key(), static_cast<std::int64_t>(v.get<std::int64_t>()));
                            }
                            else if (v.is_number_float())
                            {
                                fields.emplace(it.key(), v.get<double>());
                            }
                            else if (v.is_string())
                            {
                                fields.emplace(it.key(), v.get<std::string>());
                            }
                        }
                    }

                    Serialization::ScriptFieldReader reader(std::move(fields));
                    serializable->deserializeFields(reader);
                }
                else if (!data["fields"].empty())
                {
                    LOG_WARN("SaveGame: script type '{}' does not implement ISerializableScript; ignoring saved fields", scriptType);
                }
            }
        });
}

}  // namespace Serialization
