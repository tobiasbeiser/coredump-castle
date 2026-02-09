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

ProjectileFactory::ProjectileFactory(GameEngine::ECS::Registry &registry, AnimationFactory &animationFactory)
    : registry(registry), animationFactory(animationFactory)
{
}

void ProjectileFactory::createProjectile(const glm::vec2 &startPosition, const glm::vec2 &direction, float speed,
                                         float range, const std::string &tilesetPath, double rotation) const
{
	if (tilesetPath.empty() || tilesetPath.back() == '/' || tilesetPath.back() == '\\'
	    || tilesetPath.find(".tsj") == std::string::npos) {
		SDL_Log("[ProjectileFactory] ERROR: Cannot create projectile with invalid tileset path: '%s'",
		        tilesetPath.c_str());
		return;
	}

	const auto entity = registry.create();

	// avoid friendly fire
	registry.registerComponent(entity, EnemyTag{});
	registry.registerComponent(entity, ProjectileTag{});

	if (const auto animComponent = animationFactory.loadProjectileAnimation(tilesetPath)) {
		auto [animationComponent, spriteComponent] = *animComponent;
		registry.registerComponent<AnimationComponent>(entity, animationComponent);
		if (!registry.has<SpriteComponent>(entity)) {
			registry.registerComponent<SpriteComponent>(entity, spriteComponent);
		}
	}

	registry.registerComponent(entity, PositionComponent{.position = startPosition, .previousPosition = startPosition});
	registry.registerComponent(entity, VelocityComponent{.velocity = direction * speed});
	// Use the cached scale from ProjectileData, or default to 1.0f if not set
	const ProjectileData &data = getCachedProjectileData(tilesetPath);

	registry.registerComponent(entity,
	                           TransformComponent{.scale = data.scale, .rotation = static_cast<float>(rotation)});
	registry.registerComponent(entity, RenderComponent{.layer = 15, .flipX = false});

	SDL_FRect finalHitbox = data.hitbox;
	finalHitbox.x = data.hitbox.x;
	finalHitbox.y = data.hitbox.y;

	registry.registerComponent(entity, HurtBoxComponent{.hurtBox = finalHitbox});
	registry.registerComponent(entity, HitBoxComponent(finalHitbox));
	registry.registerComponent(entity, CollisionComponent{.collisionBox = finalHitbox});
	registry.registerComponent(entity, ProjectileComponent{.maxRange = range,
	                                                       .speed = speed,
	                                                       .spawnPosition = startPosition,
	                                                       .projectileTilesetPath = tilesetPath});
}

const ProjectileData &ProjectileFactory::getCachedProjectileData(const std::string &path)
{
	static std::unordered_map<std::string, ProjectileData> cache;

	if (path.empty() || path.back() == '/' || path.back() == '\\' || path.find(".tsj") == std::string::npos) {
		SDL_Log("[ProjectileFactory] ERROR: Invalid projectile tileset path: '%s'", path.c_str());
		static ProjectileData emptyData;
		return emptyData;
	}

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

		if (const auto scaleProp = tileset.getProp("scale")) {
			if (scaleProp->getType() == tson::Type::Float) {
				const auto scaleVal = scaleProp->getValue<float>();
				data.scale = {scaleVal, scaleVal};
			}
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