#pragma once
#include "../../Engine/ECS/Components/Core/PositionComponent.hpp"
#include "../../Engine/ECS/Components/Core/StateComponent.hpp"
#include "../../Engine/ECS/Components/Core/TransformComponent.hpp"
#include "../../Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "../../Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "../../Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "../../Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "../../Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "../../Engine/ECS/Components/Render/RenderComponent.hpp"
#include "../../Engine/ECS/Registry.hpp"
#include "../../Engine/Graphics/TextureManager.hpp"
#include "AnimationFactory.hpp"
#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include <Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp>
#include <Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp>
#include <Engine/ECS/Components/Tags/EnemyTag.hpp>
#include <Engine/ECS/Components/Tags/PassiveEnemyTag.hpp>
#include <tileson.hpp>

class EnemyFactory {
  public:
	static GameEngine::ECS::Entity createEnemy(GameEngine::ECS::Registry &registry, TextureManager &texMgr,
	                                           tson::Object &enemyObj, const glm::vec2 &levelPos, tson::Map &map,
	                                           const int layer)
	{
		const auto entity = registry.create();
		registry.registerComponent(entity, EnemyTag{});

		const auto scale = getFloatProp(enemyObj, "scale", 1.0f);
		const auto maxSpeedX = getFloatProp(enemyObj, "maxSpeedX", 50.0f);
		const auto canPatrol = getBoolProp(enemyObj, "canPatrol", false);
		const auto isPassive = getBoolProp(enemyObj, "isPassive", false);
		const auto collisionStr = getStringProp(enemyObj, "collisionBox", "0,0,0,0");

		const tson::Vector2 pos = enemyObj.getPosition();
		const glm::vec2 spawn = glm::vec2(pos.x, pos.y) + levelPos;

		registry.registerComponent(entity, PositionComponent{.position = spawn});
		registry.registerComponent(entity, VelocityComponent{.velocity = {0.0f, 0.0f}});
		registry.registerComponent(entity, AccelerationComponent{.acceleration = {0.0f, 0.0f}});
		registry.registerComponent(entity, GravityComponent{});
		registry.registerComponent(entity, GroundedComponent{});
		registry.registerComponent(entity, RenderComponent{.layer = layer, .flipX = false});
		registry.registerComponent(entity, TransformComponent{.scale = glm::vec2(scale, scale)});
		registry.registerComponent(entity, StateComponent{});
		using namespace std::chrono_literals;
		registry.registerComponent(entity,
		                           AttackComponent{.attackTimer = Timer{0.6s}, .attackCoolDownTimer = Timer{0.2s}});
		registry.registerComponent(entity, InputComponent{});
		registry.registerComponent(entity, MovementComponent{.accelerationRate = 400.0f, .maxSpeedX = 50.0f});

		if (isPassive) {
			registry.registerComponent(entity, PassiveEnemyTag{});
		}
		if (collisionStr) {
			registry.registerComponent(entity, parseCollisionBox(collisionStr.value()));
		}

		if (canPatrol) {
			if (const auto patrolRect = findPatrolRect(enemyObj, levelPos, map)) {
				const float patrolSpeed = getFloatProp(enemyObj, "patrolSpeed", 50.0f);
				registry.registerComponent(entity,
				                           PatrolComponent{.minBounds = patrolRect->min, .maxBounds = patrolRect->max});
			}
		}

		AnimationFactory::loadAnimationsFromTileset(registry, texMgr, map, enemyObj, entity);
		return entity;
	}

  private:
	struct PatrolRect {
		glm::vec2 min;
		glm::vec2 max;
	};
	static float getFloatProp(tson::Object &obj, const std::string &name, float defaultValue)
	{
		auto prop = obj.getProp(name);
		if (prop && prop->getType() == tson::Type::Float) {
			return prop->getValue<float>();
		}
		return defaultValue;
	}

	static bool getBoolProp(tson::Object &obj, const std::string &name, bool defaultValue)
	{
		auto prop = obj.getProp(name);
		if (prop && prop->getType() == tson::Type::Boolean) {
			return prop->getValue<bool>();
		}
		return defaultValue;
	}

	static std::optional<std::string> getStringProp(tson::Object &obj, const std::string &name,
	                                                const std::string defaultValue)
	{
		auto prop = obj.getProp(name);
		if (prop && prop->getType() == tson::Type::String) {
			return prop->getValue<std::string>();
		}
		if (!defaultValue.empty()) {
			return defaultValue;
		}
		return std::nullopt;
	}

	static CollisionComponent parseCollisionBox(const std::string &value)
	{
		float x, y, w, h;

		if (std::sscanf(value.c_str(), "%f,%f,%f,%f", &x, &y, &w, &h) == 4) {
			return {x, y, w, h};
		}

		return {0.0f, 0.0f, 0.0f, 0.0f};
	}

	static std::optional<PatrolRect> findPatrolRect(tson::Object &enemyObj, const glm::vec2 &levelPos, tson::Map &map)
	{
		auto *pathLayer = map.getLayer("Enemies");
		if (!pathLayer) {
			return std::nullopt;
		}

		const auto areaProp = enemyObj.getProp("area");
		if (areaProp == nullptr || areaProp->getType() != tson::Type::Object) {
			return std::nullopt;
		}
		const auto areaID = areaProp->getValue<uint32_t>();
		for (const auto &obj : pathLayer->getObjects()) {
			if (obj.getId() == areaID) {
				const tson::Vector2 pos = obj.getPosition();
				const tson::Vector2 size = obj.getSize();
				return PatrolRect{
				    .min = glm::vec2(pos.x, pos.y) + levelPos,
				    .max = glm::vec2(pos.x + size.x, pos.y + size.y) + levelPos,
				};
			}
		}

		return std::nullopt;
	}
};