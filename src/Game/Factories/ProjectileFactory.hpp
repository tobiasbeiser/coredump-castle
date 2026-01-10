#pragma once

#include "../../Engine/ECS/Components/Core/PositionComponent.hpp"
#include "../../Engine/ECS/Components/Core/TransformComponent.hpp"
#include "../../Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "../../Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "../../Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "../../Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "../../Engine/ECS/Components/Render/RenderComponent.hpp"
#include "../../Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "../../Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "../../Engine/ECS/Registry.hpp"
#include "../../Engine/Graphics/TextureManager.hpp"

#include <glm/vec2.hpp>
#include <string>

class ProjectileFactory {
  public:
	ProjectileFactory(GameEngine::ECS::Registry &registry, TextureManager &textureManager)
	    : registry(registry), textureManager(textureManager)
	{
	}

	void createProjectile(const glm::vec2 &startPosition, const glm::vec2 &direction, const float speed,
	                      const float range,
	                      const std::string &texturePath = "assets/tiled/tilesets/Bullet_Wizzart_A.png") const
	{
		const auto entity = registry.create();

		registry.registerComponent(entity, PositionComponent{.position = startPosition});
		registry.registerComponent(entity, TransformComponent{.scale = glm::vec2(1.0f, 1.0f)});

		const glm::vec2 velocity = direction * speed;
		registry.registerComponent(entity, VelocityComponent{.velocity = velocity});

		auto &texture = textureManager.getTexture(texturePath);
		constexpr glm::ivec2 textureSize = {32, 16};

		const bool flipX = (direction.x < 0);
		registry.registerComponent(entity,
		                           RenderComponent{.layer = 15, .flipX = flipX}); // Layer 15 (above enemies, below UI)

		registry.registerComponent(
		    entity, SpriteComponent{.texture = &texture, .tileSize = textureSize, .tileMapPosition = {0, 0}});

		// projectile is always dealing damage like a bodyHitBox (therefore hurtboxComponent instead of HitBoxComponent)
		constexpr SDL_FRect hitRect = {0.0f, 0.0f, static_cast<float>(textureSize.x),
		                               static_cast<float>(textureSize.y)};
		registry.registerComponent(entity, HurtBoxComponent{.hurtBox = hitRect});

		registry.registerComponent(entity, EnemyTag{});
		registry.registerComponent(entity, ProjectileComponent{.maxRange = range, .speed = speed});

		registry.registerComponent(entity, AnimationComponent{});
		auto &animComp = registry.get<AnimationComponent>(entity);
		animComp.addAnimationFromSpritesheet(AnimationType::Idle, 4, std::chrono::duration<float>(0.4f), &texture,
		                                     textureSize, {0, 0});
		animComp.setAnimation(AnimationType::Idle);
	}

  private:
	GameEngine::ECS::Registry &registry;
	TextureManager &textureManager;
};
