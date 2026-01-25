#include "Game/Factories/ProjectileFactory.hpp"

#include "AnimationFactory.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/ProjectileTag.hpp"
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
	reg.registerComponent(entity, ProjectileTag{});

	AnimationFactory::loadProjectileAnimation(reg, textureManager, tilesetPath, entity);

	reg.registerComponent(entity, PositionComponent{.position = startPosition, .previousPosition = startPosition});
	reg.registerComponent(entity, VelocityComponent{.velocity = direction * speed});
	reg.registerComponent(entity,
	                      TransformComponent{.scale = glm::vec2(1.0f, 1.0f), .rotation = static_cast<float>(rotation)});
	reg.registerComponent(entity, RenderComponent{.layer = 15, .flipX = false});

	const ProjectileData &data = getCachedProjectileData(tilesetPath);
	if (!reg.has<SpriteComponent>(entity) && !data.texturePath.empty()) {
		auto &texture = textureManager.get().getTexture(data.texturePath);
		reg.registerComponent(
		    entity, SpriteComponent{.texture = &texture, .tileSize = data.tileSize, .tileMapPosition = {0, 0}});
	}

	SDL_FRect finalHitbox = data.hitbox;
	finalHitbox.x = data.hitbox.x;
	finalHitbox.y = data.hitbox.y;

	reg.registerComponent(entity, HurtBoxComponent{.hurtBox = finalHitbox});
	reg.registerComponent(entity, HitBoxComponent{.bodyHitBox = finalHitbox, .weaponHitBox = {0.0f, 0.0f, 0.0f, 0.0f}});
	reg.registerComponent(entity, CollisionComponent{.collisionBox = finalHitbox});
	reg.registerComponent(entity, ProjectileComponent{.maxRange = range,
	                                                  .speed = speed,
	                                                  .spawnPosition = startPosition,
	                                                  .projectileTilesetPath = tilesetPath});
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

		data.tileSize = {tileset.getTileSize().x, tileset.getTileSize().y};
		if (!tileset.getImage().empty()) {
			const std::string tilesetDir = path.substr(0, path.find_last_of("/\\") + 1);
			data.texturePath = tilesetDir + tileset.getImage().string();
		}

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