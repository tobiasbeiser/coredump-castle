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
		for (const auto entity : view) {
			auto &anim = view.get<AnimationComponent>(entity);
			auto &[texture, TileSize, TileMapPosition] = view.get<SpriteComponent>(entity);

			if (registry.has<StateComponent>(entity)) {
				anim.previousAnimation = anim.currentAnimation;
				anim.currentAnimation = registry.get<StateComponent>(entity).state;
			}

			if (anim.currentAnimation != -1) {

				auto &animation = anim.animations.at(anim.currentAnimation);

				if (anim.previousAnimation != anim.currentAnimation) {
					animation.start();
				}

				animation.step(stepSize);

				// Update tile map position based on current animation frame#
				auto animFrames = anim.frames.at(anim.currentAnimation);
				const int currentFrame = animation.currentFrame();
				texture = animFrames.at(currentFrame).texture;
				TileSize = anim.frames.at(anim.currentAnimation).at(animation.currentFrame()).TileSize;
				TileMapPosition =
				    anim.frames.at(anim.currentAnimation).at(animation.currentFrame()).TileMapPosition;
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems
