#pragma once

#include <Entity.h>
#include <Vec2.h>

#include <CNativeScript.h>

class World;

namespace Example
{

Entity spawnBarrel(World& world, const Vec2& position);

class BarrelScript final : public Components::INativeScript
{
public:
	static constexpr const char* kScriptName = "BarrelScript";

	void onCreate(Entity self, World& world) override;
	void onUpdate(float deltaTime, Entity self, World& world) override;
};

}  // namespace Example
