#pragma once

#include "../../Engine/Debug/DebugSystem.hpp"
#include "AnimationFactory.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"

#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include <tileson.hpp>

class EnemyFactory {
  public:
	EnemyFactory(GameEngine::ECS::Registry &registry, AnimationFactory &animationFactory)
	    : registry(registry), animationFactory(animationFactory)
	{
	}

	EnemyFactory(EnemyFactory &&) = delete;
	EnemyFactory(const EnemyFactory &) = delete;
	EnemyFactory &operator=(const EnemyFactory &) = delete;
	EnemyFactory &operator=(EnemyFactory &&) = delete;
	~EnemyFactory() = default;

	void setMap(tson::Map &tsonMap) { map = &tsonMap; }
	GameEngine::ECS::Entity createEnemy(tson::Object &enemyObj, const glm::vec2 &levelPos, const int layer) const
	{
		if (!map) {
			throw std::runtime_error("Map is null");
		}

		const auto entity = registry.create();
		registry.registerComponent(entity, EnemyTag{});

		const auto scale = getFloatProp(enemyObj, "scale", 1.0f);
		const auto maxSpeedX = getFloatProp(enemyObj, "maxSpeedX", 50.0f);
		const auto chaseSpeed = getFloatProp(enemyObj, "chaseSpeed", 0.0f);
		const auto verticalDetectionHeight = getFloatProp(enemyObj, "verticalDetectionHeight", 50.0f);
		const auto canPatrol = getBoolProp(enemyObj, "canPatrol");
		const auto collisionStr = getStringProp(enemyObj, "collisionBox", "0,0,0,0");
		const auto hitBoxStr = getStringProp(enemyObj, "hitBox", "");
		const auto maxHealth = getIntProp(enemyObj, "maxHealth", 100);
		const auto attackCooldown = getFloatProp(enemyObj, "attackCooldown", 0.5);
		const auto reactionDelay = getFloatProp(enemyObj, "reactionDelay", 0.5);

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

		registry.registerComponent(entity, InputComponent{});
		registry.registerComponent(entity, MovementComponent{.maxSpeedX = maxSpeedX});
		registry.registerComponent(entity, BlinkComponent{});

		SDL_FRect weaponHitBox = {0.f, 0.f, 0.f, 0.f};
		if (hitBoxStr) {
			weaponHitBox = parseCollisionBox(hitBoxStr.value());
		}

		if (collisionStr) {
			const auto collisionBox = parseCollisionBox(collisionStr.value());
			registry.registerComponent(entity, HitBoxComponent{.bodyHitBox = parseCollisionBox(collisionStr.value()),
			                                                   .weaponHitBox = weaponHitBox});
			registry.registerComponent(entity, CollisionComponent{collisionBox});
			registry.registerComponent(entity, HurtBoxComponent{collisionBox});
		}

		if (canPatrol) {
			if (const auto patrolRect = findPatrolRect(enemyObj, levelPos, *map)) {
				registry.registerComponent(entity,
				                           PatrolComponent{.minBounds = patrolRect->min, .maxBounds = patrolRect->max});
			}
			if (chaseSpeed > 0.0f) {
				registry.registerComponent(
				    entity, ChaseComponent{.verticalDetectionHeight = verticalDetectionHeight,
				                           .chaseSpeed = chaseSpeed,
				                           .originalPatrolSpeed = maxSpeedX,
				                           .reactionDelay = Timer{std::chrono::duration<float>(reactionDelay)}});
			}
		}

		animationFactory.loadAnimationsFromTileset(enemyObj, entity);

		using namespace std::chrono_literals;
		std::chrono::duration<float> attackDuration = 100ms;
		std::chrono::duration<float> invulnerabilityTime = 300ms;

		if (registry.has<AnimationComponent>(entity)) {
			auto &animComp = registry.get<AnimationComponent>(entity);
			if (animComp.hasAnimation(AnimationType::Attacking)) {
				attackDuration = animComp.getAnimationDuration(AnimationType::Attacking);
			}
			if (animComp.hasAnimation(AnimationType::Hurting)) {
				invulnerabilityTime = animComp.getAnimationDuration(AnimationType::Hurting);
			}
		}

		registry.registerComponent(entity, HealthComponent{maxHealth, maxHealth, invulnerabilityTime});

		registry.registerComponent(
		    entity, AttackComponent{.attackTimer = Timer{attackDuration},
		                            .attackCoolDownTimer = Timer{std::chrono::duration<float>(attackCooldown)}});

		return entity;
	}

  private:
	GameEngine::ECS::Registry &registry;
	AnimationFactory &animationFactory;
	tson::Map *map = nullptr;

	struct PatrolRect {
		glm::vec2 min;
		glm::vec2 max;
	};
	static float getFloatProp(tson::Object &obj, const std::string &name, const float defaultValue)
	{
		const auto prop = obj.getProp(name);
		if (prop == nullptr) {
			return defaultValue;
		}
		if (prop->getType() != tson::Type::Float) {
			return defaultValue;
		}
		return prop->getValue<float>();
	}

	static int getIntProp(tson::Object &obj, const std::string &name, const int defaultValue)
	{
		const auto prop = obj.getProp(name);
		if (prop == nullptr) {
			return defaultValue;
		}
		if (prop->getType() != tson::Type::Int) {
			return defaultValue;
		}
		return prop->getValue<int>();
	}

	static bool getBoolProp(tson::Object &obj, const std::string &name)
	{
		const auto prop = obj.getProp(name);
		if (prop == nullptr) {
			return false;
		}
		if (prop->getType() != tson::Type::Boolean) {
			return false;
		}
		return prop->getValue<bool>();
	}

	static std::optional<std::string> getStringProp(tson::Object &obj, const std::string &name,
	                                                const std::string &defaultValue)
	{
		const auto prop = obj.getProp(name);
		if (!prop) {
			return defaultValue.empty() ? std::nullopt : std::optional{defaultValue};
		}
		if (prop->getType() != tson::Type::String) {
			return defaultValue.empty() ? std::nullopt : std::optional{defaultValue};
		}
		return prop->getValue<std::string>();
	}

	static SDL_FRect parseCollisionBox(const std::string &value)
	{
		float x = 0.f, y = 0.f, w = 0.f, h = 0.f;
		std::istringstream iss(value);
		char c1 = 0, c2 = 0, c3 = 0;
		if (iss >> x >> c1 >> y >> c2 >> w >> c3 >> h && c1 == ',' && c2 == ',' && c3 == ',') {
			return {x, y, w, h};
		}
		return {0.f, 0.f, 0.f, 0.f};
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