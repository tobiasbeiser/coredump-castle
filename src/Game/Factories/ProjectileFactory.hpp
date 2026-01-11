#pragma once

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/TextureManager.hpp"

#include <glm/vec2.hpp>
#include <string>

struct ProjectileConfig {
	std::string texturePath = "assets/tiled/tilesets/Bullet_Wizzart_A.png";
	glm::ivec2 frameSize = {32, 16};
	int frameCount = 4;
	float animationDuration = 0.6f; // Total animation duration in seconds
};

class ProjectileFactory {
  public:
	ProjectileFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
	    : registry(registry), textureManager(textureManager)
	{
	}

	void createProjectile(const glm::vec2 &startPosition, const glm::vec2 &direction, const float speed,
	                      const float range, const double rotation = 0.0, const ProjectileConfig &config = {}) const
	{
		const auto entity = registry.create();

		registry.registerComponent(entity, PositionComponent{.position = startPosition});
		registry.registerComponent(
		    entity, TransformComponent{.scale = glm::vec2(1.0f, 1.0f), .rotation = static_cast<float>(rotation)});

		const glm::vec2 velocity = direction * speed;
		registry.registerComponent(entity, VelocityComponent{.velocity = velocity});

		auto &texture = textureManager.getTexture(config.texturePath);

		const bool flipX = false;
		registry.registerComponent(entity,
		                           RenderComponent{.layer = 15, .flipX = flipX}); // Layer 15 (above enemies, below UI)

		registry.registerComponent(
		    entity, SpriteComponent{.texture = &texture, .tileSize = config.frameSize, .tileMapPosition = {0, 0}});

		// Projectile is always dealing damage like a bodyHitBox (therefore hurtboxComponent instead of HitBoxComponent)
		const SDL_FRect hitRect = {flipX ? 5.0f : -5.0f, 0.0f, 30, 16};
		registry.registerComponent(entity, HurtBoxComponent{.hurtBox = hitRect});
		registry.registerComponent(entity, CollisionComponent{.collisionBox = hitRect});

		registry.registerComponent(entity, EnemyTag{});
		registry.registerComponent(
		    entity, ProjectileComponent{.maxRange = range, .speed = speed, .spawnPosition = startPosition});

		registry.registerComponent(entity, AnimationComponent{});

		// not using AnimationFactor since not bounded to map and object
		auto &animComp = registry.get<AnimationComponent>(entity);
		animComp.addAnimationFromSpritesheet(AnimationType::Idle, config.frameCount,
		                                     std::chrono::duration<float>(config.animationDuration), &texture,
		                                     config.frameSize, {0, 0});
		animComp.setAnimation(AnimationType::Idle);
	}

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
};
