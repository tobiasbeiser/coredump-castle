#pragma once

#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include <SDL3/SDL.h>

namespace GameEngine::ECS::Systems {
class AnimationSystem {
  public:
	static void Update(Registry &registry, float stepSize)
	{
		auto view = registry.view<AnimationComponent, SpriteComponent>();
		for (auto entity : view) {
			auto &anim = view.get<AnimationComponent>(entity);
			auto &tile = view.get<SpriteComponent>(entity);

			if (registry.has<StateComponent>(entity)) {
				anim.previousAnimation = anim.currentAnimation;
				anim.currentAnimation = anim.stateToAnimationIndex.at(registry.get<StateComponent>(entity).state);
			}
			
			tile.TileMapPosition = {0, 0}; // Default to first frame if no animation is active
			if (anim.currentAnimation != -1) {

				auto &animation = anim.animations.at(anim.currentAnimation);

				if (anim.previousAnimation != anim.currentAnimation) {
					animation.start();
				}

				animation.step(stepSize);

				// Update tile map position based on current animation frame
				tile.TileMapPosition.x = anim.animations.at(anim.currentAnimation).currentFrame() * tile.TileSize.x;
				tile.TileMapPosition.y = anim.currentAnimation * tile.TileSize.y;
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems
