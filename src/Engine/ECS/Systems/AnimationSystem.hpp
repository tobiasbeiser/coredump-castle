#pragma once

#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {

class AnimationSystem {
  public:
	static void update(Registry &registry, float stepSize)
	{
		auto view = registry.view<AnimationComponent, SpriteComponent>();
		for (const auto entity : view) {
			auto &anim = view.get<AnimationComponent>(entity);
			auto &[texture, TileSize, TileMapPosition] = view.get<SpriteComponent>(entity);

			// Determine current animation from state
			if (registry.has<StateComponent>(entity)) {
				anim.previousAnimation = anim.currentAnimation;
				const auto &stateComp = registry.get<StateComponent>(entity);
				anim.currentAnimation = static_cast<int>(stateComp.state);
			}

			if (anim.currentAnimation != -1 && anim.animations.count(anim.currentAnimation) > 0) {
				auto &animation = anim.animations.at(anim.currentAnimation);
				auto &animFrames = anim.frames.at(anim.currentAnimation);

				// Restart animation if changed
				if (anim.previousAnimation != anim.currentAnimation) {
					animation.start();
				}

				animation.step(stepSize);

				const int currentFrame = animation.currentFrame();
				texture = animFrames[currentFrame].texture;
				TileSize = animFrames[currentFrame].tileSize;
				TileMapPosition = animFrames[currentFrame].tileMapPosition;
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems
