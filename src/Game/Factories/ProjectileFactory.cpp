#include "Game/Factories/ProjectileFactory.hpp"

#include "AnimationFactory.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "tileson.hpp"

#include <iostream>
#include <unordered_map>

ProjectileFactory::ProjectileFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
    : registry(registry), textureManager(textureManager)
{
}

void ProjectileFactory::createProjectile(const glm::vec2 &startPosition, const glm::vec2 &direction, float speed,
                                         float range, const std::string &tilesetPath, double rotation) const
{
	auto &reg = registry.get();
	const auto entity = reg.create();

	// avoid friendly fire
	reg.registerComponent(entity, EnemyTag{});

	AnimationFactory::loadProjectileAnimation(reg, textureManager, tilesetPath, entity);

	reg.registerComponent(entity, PositionComponent{.position = startPosition, .previousPosition = startPosition});
	reg.registerComponent(entity, VelocityComponent{.velocity = direction * speed});
	reg.registerComponent(entity,
	                      TransformComponent{.scale = glm::vec2(1.0f, 1.0f), .rotation = static_cast<float>(rotation)});
	reg.registerComponent(entity, RenderComponent{.layer = 15, .flipX = false});

	const ProjectileData &data = getCachedProjectileData(tilesetPath);

	SDL_FRect finalHitbox = data.hitbox;
	finalHitbox.x = data.hitbox.x;
	finalHitbox.y = data.hitbox.y;

	reg.registerComponent(entity, HurtBoxComponent{.hurtBox = finalHitbox});
	reg.registerComponent(entity, CollisionComponent{.collisionBox = finalHitbox});
	reg.registerComponent(entity, ProjectileComponent{.maxRange = range,
	                                                  .speed = speed,
	                                                  .spawnPosition = startPosition,
	                                                  .projectileTilesetPath = tilesetPath,
	                                                  .projectileSize = {finalHitbox.w, finalHitbox.h},
	                                                  .spriteSize = data.spriteSize,
	                                                  .hitboxOffset = {data.hitbox.x, data.hitbox.y}});
}

const ProjectileData &ProjectileFactory::getCachedProjectileData(const std::string &path)
{
	static std::unordered_map<std::string, ProjectileData> cache;

	if (auto it = cache.find(path); it != cache.end()) {
		return it->second;
	}

	ProjectileData data;

	tson::Tileset tileset;
	tson::Json11 json;

	if (json.parse(path)) {
		tileset.parse(json, nullptr);

		data.spriteSize = {static_cast<float>(tileset.getTileSize().x), static_cast<float>(tileset.getTileSize().y)};

		if (!tileset.getTiles().empty()) {
			auto &firstTile = tileset.getTiles()[0];
			auto &objectGroup = firstTile.getObjectgroup();

			for (const auto &obj : objectGroup.getObjects()) {
				if (obj.getObjectType() == tson::ObjectType::Rectangle) {
					data.hitbox.x = static_cast<float>(obj.getPosition().x);
					data.hitbox.y = static_cast<float>(obj.getPosition().y);
					data.hitbox.w = static_cast<float>(obj.getSize().x);
					data.hitbox.h = static_cast<float>(obj.getSize().y);
					break;
				}
			}
		}
	} else {
		std::cerr << "[ProjectileFactory] Failed to parse tileset: " << path << std::endl;
	}

	return cache.emplace(path, data).first->second;
}