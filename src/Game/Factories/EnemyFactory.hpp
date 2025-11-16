#pragma once
#include "../../Engine/ECS/Components/Core/PositionComponent.hpp"
#include "../../Engine/ECS/Components/Core/StateComponent.hpp"
#include "../../Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "../../Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "../../Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "../../Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "../../Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "../../Engine/ECS/Components/Render/RenderComponent.hpp"
#include "../../Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "../../Engine/ECS/Registry.hpp"
#include "../../Engine/Graphics/TextureManager.hpp"
#include "AnimationFactory.hpp"
#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"

#include <tileson.hpp>

class EnemyFactory {
  public:
	static GameEngine::ECS::Entity createEnemy(GameEngine::ECS::Registry &registry, TextureManager &texMgr,
	                                           tson::Object &enemyObj, const glm::vec2 &levelPos, tson::Map &map)
	{
		const auto entity = registry.create();

		const tson::Vector2 pos = enemyObj.getPosition();
		const glm::vec2 spawn = glm::vec2(pos.x, pos.y) + levelPos;

		registry.registerComponent(entity, PositionComponent{.position = spawn});
		registry.registerComponent(entity, VelocityComponent{.velocity = {0.0f, 0.0f}});
		registry.registerComponent(entity, AccelerationComponent{.acceleration = {0.0f, 0.0f}});
		registry.registerComponent(entity, MovementComponent{});
		registry.registerComponent(entity, GravityComponent{});
		registry.registerComponent(entity, GroundedComponent{});
		registry.registerComponent(entity, CollisionComponent{6, 8, 12, 12});
		registry.registerComponent(entity, RenderComponent{.layer = 90, .flipX = false});
		registry.registerComponent(entity, StateComponent{});

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
};