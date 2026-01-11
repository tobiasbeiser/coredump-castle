#pragma once

#include "AnimationFactory.hpp"
#include "Engine/DebugSystem.hpp"
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
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/View.hpp"
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
		const auto chaseSpeed = getFloatProp(enemyObj, "chaseSpeed", 0.0f);
		const auto verticalDetectionHeight = getFloatProp(enemyObj, "verticalDetectionHeight", 50.0f);
		const auto canPatrol = getBoolProp(enemyObj, "canPatrol");
		const auto collisionStr = getStringProp(enemyObj, "collisionBox", "0,0,0,0");
		const auto hitBoxStr = getStringProp(enemyObj, "hitBox", "");
		const auto maxHealth = getIntProp(enemyObj, "maxHealth", 100);
		const auto attackCooldown = getFloatProp(enemyObj, "attackCooldown", 0.5);
		const auto reactionDelay = getFloatProp(enemyObj, "reactionDelay", 0.5);
		const auto isRangedEnemy = getBoolProp(enemyObj, "isRangedEnemy");
		const auto projectileSpeed = getFloatProp(enemyObj, "projectileSpeed", 300.0f);
		const auto detectionRange = getFloatProp(enemyObj, "detectionRange", 300.0f);
		glm::vec2 projectileOffset = {0.0f, 0.0f};
		if (const auto offsetStr = getStringProp(enemyObj, "projectileOffset", "")) {
			const auto parsed = parseCollisionBox(offsetStr.value());
			projectileOffset = {parsed.x, parsed.y};
		}

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
			if (const auto patrolRect = findPatrolRect(enemyObj, levelPos, map)) {
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
		if (isRangedEnemy) {
			registry.registerComponent(
			    entity, ChaseComponent{.verticalDetectionHeight = verticalDetectionHeight,
			                           .chaseSpeed = chaseSpeed,
			                           .originalPatrolSpeed = maxSpeedX,
			                           .reactionDelay = Timer{std::chrono::duration<float>(reactionDelay)}});
			registry.registerComponent(entity, RangedAttackComponent{
			                                       .detectionRange = detectionRange,
			                                       .projectileSpeed = projectileSpeed,
			                                       .projectileOffset = projectileOffset,
			                                   });
			registry.registerComponent(entity, ProjectileComponent{.maxRange = 2000.0f, .speed = projectileSpeed});
		}

		AnimationFactory::loadAnimationsFromTileset(registry, texMgr, map, enemyObj, entity);

		using namespace std::chrono_literals;
		std::chrono::duration<float> attackDuration = 1s;
		std::chrono::duration<float> invulnerabilityTime = 1s;

		if (registry.has<AnimationComponent>(entity)) {
			auto &animComp = registry.get<AnimationComponent>(entity);
			if (animComp.hasAnimation(AnimationType::Attacking)) {
				attackDuration = animComp.getAnimationDuration(AnimationType::Attacking);
			}

			if (animComp.hasAnimation(AnimationType::Charge)) {
				attackDuration = animComp.getAnimationDuration(AnimationType::Charge);
				if (animComp.hasAnimation(AnimationType::Fire)) {
					attackDuration += animComp.getAnimationDuration(AnimationType::Fire);
				}
			} else if (animComp.hasAnimation(AnimationType::Fire)) {
				attackDuration = animComp.getAnimationDuration(AnimationType::Fire);
			}
			if (animComp.hasAnimation(AnimationType::Hurting)) {
				invulnerabilityTime = animComp.getAnimationDuration(AnimationType::Hurting);
			}
		}

		registry.registerComponent(entity, HealthComponent{maxHealth, maxHealth, invulnerabilityTime});

		registry.registerComponent(
		    entity, AttackComponent{.attackTimer = Timer{attackDuration},
		                            .attackCoolDownTimer = Timer{std::chrono::duration<float>(attackCooldown)}});

		ensureSafeSpawn(registry, entity);

		return entity;
	}

  private:
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
	static void ensureSafeSpawn(GameEngine::ECS::Registry &registry, const GameEngine::ECS::Entity entity)
	{
		if (!registry.has<PositionComponent>(entity) || !registry.has<CollisionComponent>(entity)) {
			return;
		}

		auto &entityPos = registry.get<PositionComponent>(entity);
		const auto &entityCollision = registry.get<CollisionComponent>(entity).collisionBox;
		glm::vec2 scale = {1.0f, 1.0f};
		if (registry.has<TransformComponent>(entity)) {
			scale = registry.get<TransformComponent>(entity).scale;
		}

		int maxAttempts = 100;
		bool collision = true;

		while (collision && maxAttempts > 0) {
			collision = false;
			SDL_FRect enemyRect = {entityPos.position.x + entityCollision.x * scale.x,
			                       entityPos.position.y + entityCollision.y * scale.y, entityCollision.w * scale.x,
			                       entityCollision.h * scale.y};

			for (auto staticColliders = registry.view<PositionComponent, CollisionComponent>()
			                                .exclude<EnemyTag>()
			                                .exclude<PlayerTag>()
			                                .exclude<ProjectileComponent>();
			     const auto staticCollider : staticColliders) {

				const auto &staticColliderPos = staticColliders.get<PositionComponent>(staticCollider);
				const auto &staticColliderCollisionBox =
				    staticColliders.get<CollisionComponent>(staticCollider).collisionBox;
				glm::vec2 staticColliderScale = {1.0f, 1.0f};
				if (registry.has<TransformComponent>(staticCollider)) {
					staticColliderScale = registry.get<TransformComponent>(staticCollider).scale;
				}

				SDL_FRect staticColliderRect = {
				    staticColliderPos.position.x + staticColliderCollisionBox.x * staticColliderScale.x,
				    staticColliderPos.position.y + staticColliderCollisionBox.y * staticColliderScale.y,
				    staticColliderCollisionBox.w * staticColliderScale.x,
				    staticColliderCollisionBox.h * staticColliderScale.y};

				if (SDL_HasRectIntersectionFloat(&enemyRect, &staticColliderRect)) {
					collision = true;
					entityPos.position.y -= 2.0f;
					break;
				}
			}
			maxAttempts--;
		}

		if (maxAttempts == 0) {
			SDL_Log("Warning: Could not safe-spawn enemy entity %u", static_cast<unsigned int>(entity));
		}
	}
};