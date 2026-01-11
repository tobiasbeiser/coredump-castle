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
		const auto entity = registry.create();

		registry.registerComponent(entity, PositionComponent{.position = startPosition});
		registry.registerComponent(
		    entity, TransformComponent{.scale = glm::vec2(1.0f, 1.0f), .rotation = static_cast<float>(rotation)});

		const glm::vec2 velocity = direction * speed;
		registry.registerComponent(entity, VelocityComponent{.velocity = velocity});

		registry.registerComponent(entity, RenderComponent{.layer = 15, .flipX = false});

		// Projectile is always dealing damage like a bodyHitBox (therefore hurtboxComponent instead of HitBoxComponent)
		SDL_FRect hitRect = {(direction.x < 0.0f) ? 5.0f : -5.0f, 0.0f, 30, 16};
		registry.registerComponent(entity, HurtBoxComponent{.hurtBox = hitRect});
		registry.registerComponent(entity, CollisionComponent{.collisionBox = hitRect});

		registry.registerComponent(entity, EnemyTag{});
		registry.registerComponent(
		    entity, ProjectileComponent{.maxRange = range, .speed = speed, .spawnPosition = startPosition});

		// Load animation from tileset using AnimationFactory
		AnimationFactory::loadProjectileAnimation(registry, textureManager, tilesetPath, entity);
	}

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
};
