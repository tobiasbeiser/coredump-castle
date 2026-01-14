#pragma once

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
#include "Engine/Graphics/TextureManager.hpp"

#include <glm/vec2.hpp>
#include <string>

class ProjectileFactory {
  public:
	ProjectileFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
	    : registry(registry), textureManager(textureManager)
	{
	}

	void createProjectile(const glm::vec2 &startPosition, const glm::vec2 &direction, const float speed,
	                      const float range, const std::string &tilesetPath, const double rotation = 0.0) const
	{
		auto &reg = registry.get();
		const auto entity = reg.create();

		// Load animation from tileset using AnimationFactory
		AnimationFactory::loadProjectileAnimation(registry, textureManager, tilesetPath, entity);

		reg.registerComponent(entity, PositionComponent{.position = startPosition});
		reg.registerComponent(
		    entity, TransformComponent{.scale = glm::vec2(1.0f, 1.0f), .rotation = static_cast<float>(rotation)});

		const glm::vec2 velocity = direction * speed;
		reg.registerComponent(entity, VelocityComponent{.velocity = velocity});

		reg.registerComponent(entity, RenderComponent{.layer = 15, .flipX = false});

		float hitBoxX = 0.0f;
		float hitBoxY = 0.0f;
		if (reg.has<SpriteComponent>(entity)) {
			const auto &sprite = reg.get<SpriteComponent>(entity);
			hitBoxX = (static_cast<float>(sprite.tileSize.x)) / 2.0f;
			hitBoxY = (static_cast<float>(sprite.tileSize.y)) / 2.0f;
		}

		const SDL_FRect hitRect = {hitBoxX, hitBoxY, 3, 3};
		reg.registerComponent(entity, HurtBoxComponent{.hurtBox = hitRect});
		reg.registerComponent(entity, CollisionComponent{.collisionBox = hitRect});

		reg.registerComponent(entity, EnemyTag{});
		reg.registerComponent(entity, ProjectileComponent{.maxRange = range,
		                                                  .speed = speed,
		                                                  .spawnPosition = startPosition,
		                                                  .projectileSize = {hitRect.w, hitRect.h}});
	}

  private:
	std::reference_wrapper<GameEngine::ECS::Registry> registry;
	std::reference_wrapper<TextureManager> textureManager;
};
