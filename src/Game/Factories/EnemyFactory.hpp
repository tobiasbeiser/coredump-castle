#pragma once
#include "../../Engine/ECS/Components/Core/PositionComponent.hpp"
#include "../../Engine/ECS/Components/Core/StateComponent.hpp"
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
#include "Engine/ECS/Components/Gameplay/Enemy/EnemyTypeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include <Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp>
#include <Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp>
#include <Engine/ECS/Components/Gameplay/Combat/PushbackComponent.hpp>
#include <Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp>
#include <Engine/ECS/Components/Tags/EnemyTag.hpp>
#include <tileson.hpp>

class EnemyFactory {
  public:
	static GameEngine::ECS::Entity createEnemy(GameEngine::ECS::Registry &registry, TextureManager &texMgr,
	                                           tson::Object &enemyObj, const glm::vec2 &levelPos, tson::Map &map)
	{
		const auto entity = registry.create();
		registry.registerComponent(entity, EnemyTag{});

		const auto enemyTypeProp = enemyObj.getProp("enemyType");
		auto enemyType = EnemyType::Base;
		if (enemyTypeProp != nullptr && enemyTypeProp->getType() == tson::Type::String) {
			if (const auto typeStr = enemyTypeProp->getValue<std::string>(); typeStr == "Long") {
				enemyType = EnemyType::Long;
			} else if (typeStr == "Advanced") {
				enemyType = EnemyType::Advanced;
			} else if (typeStr == "Boss") {
				enemyType = EnemyType::Boss;
			}
		}

		const tson::Vector2 pos = enemyObj.getPosition();
		const glm::vec2 spawn = glm::vec2(pos.x, pos.y) + levelPos;
		const float scale = getScaleForEnemyType(enemyType);

		registry.registerComponent(entity, PositionComponent{.position = spawn});
		registry.registerComponent(entity, VelocityComponent{.velocity = {0.0f, 0.0f}});
		registry.registerComponent(entity, AccelerationComponent{.acceleration = {0.0f, 0.0f}});
		registry.registerComponent(entity, MovementComponent{});
		registry.registerComponent(entity, GravityComponent{});
		registry.registerComponent(entity, GroundedComponent{});
		registry.registerComponent(entity, RenderComponent{.layer = 90, .flipX = false, .scale = scale});
		registry.registerComponent(entity, StateComponent{});
		registry.registerComponent(entity, AttackComponent{.attackTimer = {0.6f}, .attackCoolDownTimer = {0.2f}});
		registry.registerComponent(entity, BlinkComponent{0.5f});

		configureEnemyByType(registry, entity, enemyType, scale, enemyObj, levelPos, map);

		if (const auto prop = enemyObj.getProp("animations"); prop != nullptr && prop->getType() == tson::Type::File) {
			const std::filesystem::path basePath = "assets/tiled/tilesets/";
			const auto tilesetPath = basePath / prop->getValue<std::filesystem::path>();
			tson::Tileset tileset;
			tson::Json11 json;
			json.parse(tilesetPath);
			tileset.parse(json, &map);

			const AnimationFactory animFactory(registry, texMgr);

			for (auto tile : tileset.getTiles()) {
				if (auto anim = tile.getAnimation(); anim.any()) {
					animFactory.loadAnimation(&tile, entity);
				}
			}
		}
		return entity;
	}

  private:
	struct PatrolRect {
		glm::vec2 min;
		glm::vec2 max;
	};
	static float getScaleForEnemyType(EnemyType type)
	{
		switch (type) {
		case EnemyType::Base:
			return 1.8f;
		default:
			return 1.0f;
		}
	}
	static void configureEnemyByType(GameEngine::ECS::Registry &registry, const GameEngine::ECS::Entity entity,
	                                 const EnemyType type, const float scale, tson::Object &enemyObj,
	                                 const glm::vec2 &levelPos, tson::Map &map)
	{
		switch (type) {
		case EnemyType::Base:
			if (const auto patrolRect = findPatrolRect(enemyObj, levelPos, map)) {
				registry.registerComponent(entity, PatrolComponent{.minBounds = patrolRect->min,
				                                                   .maxBounds = patrolRect->max,
				                                                   .speed = 50.0f,
				                                                   .direction = 1});
			}
			registry.registerComponent(entity, CollisionComponent{6 * scale, 10 * scale, 21 * scale, 13 * scale});
			registry.registerComponent(
			    entity, HitBoxComponent{.hitbox = {6 * scale, 10 * scale, 21 * scale, 13 * scale}});
			registry.registerComponent(entity, PushbackComponent{});
			break;
		case EnemyType::Long:
			break;
		case EnemyType::Advanced:
			break;
		case EnemyType::Boss:
			break;
		}
	}
	static std::optional<PatrolRect> findPatrolRect(tson::Object &enemyObj, const glm::vec2 &levelPos, tson::Map &map)
	{
		auto *pathLayer = map.getLayer("Entities");
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