#pragma once

#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {

class AnimationSystem {
  public:
	static void handleJumping(AnimationComponent &anim)
	{
		switch (anim.previousAnimation) {
		case AnimationType::None:
		case AnimationType::Idle:
		case AnimationType::Walking:
		case AnimationType::Landing:
		case AnimationType::Attacking:
			anim.currentAnimation = AnimationType::JumpStart;
			break;
		case AnimationType::JumpStart:
			if (!anim.hasFinished(anim.previousAnimation)) {
				anim.currentAnimation = AnimationType::JumpStart;
			} else {
				anim.currentAnimation = AnimationType::FlyingUp;
			}
			break;
		case AnimationType::FlyingUp:
			anim.currentAnimation = AnimationType::FlyingUp;
			break;
		case AnimationType::JumpReload:
			if (!anim.hasFinished(anim.previousAnimation)) {
				anim.currentAnimation = AnimationType::JumpReload;
			} else {
				anim.currentAnimation = AnimationType::FlyingUp;
			}
			break;
		case AnimationType::Falling:
			anim.currentAnimation = AnimationType::JumpReload;
			break;
		}
	}
	static void handleFalling(AnimationComponent &anim)
	{
		switch (anim.previousAnimation) {

		case AnimationType::None:
		case AnimationType::Idle:
		case AnimationType::Walking:
		case AnimationType::JumpStart:
		case AnimationType::FlyingUp:
		case AnimationType::Attacking:
		case AnimationType::Landing:
			anim.currentAnimation = AnimationType::JumpReload;
			break;
		case AnimationType::JumpReload:
			if (!anim.hasFinished(anim.previousAnimation)) {
				anim.currentAnimation = AnimationType::JumpReload;
			} else {
				anim.currentAnimation = AnimationType::Falling;
			}
			break;
		case AnimationType::Falling:
			anim.currentAnimation = AnimationType::Falling;
			break;
		}
		anim.currentAnimation = AnimationType::Falling;
	}
	static void handleFalling(AnimationComponent &anim, const AnimationType fallback)
	{
		if (anim.previousAnimation == AnimationType::Falling) {
			anim.currentAnimation = AnimationType::Landing;
		} else if (anim.previousAnimation == AnimationType::Landing && !anim.hasFinished(anim.previousAnimation)) {
			anim.currentAnimation = AnimationType::Landing;
		} else {
			anim.currentAnimation = fallback;
		}
	}
	static void update(Registry &registry, const float deltaTime)
	{
		for (auto view = registry.view<AnimationComponent, SpriteComponent>(); const auto entity : view) {
			auto &anim = view.get<AnimationComponent>(entity);
			auto &[texture, TileSize, TileMapPosition] = view.get<SpriteComponent>(entity);

			// Determine current animation from state
			if (registry.has<StateComponent>(entity)) {

				anim.previousAnimation = anim.currentAnimation;
				switch (const auto &[state] = registry.get<StateComponent>(entity); state) {

				case State::Idle:
					handleFalling(anim, AnimationType::Idle);
					break;
				case State::Walking:
					handleFalling(anim, AnimationType::Walking);
					break;
				case State::Jumping:
					handleJumping(anim);
					break;
				case State::Falling:
					handleFalling(anim);
					break;
				case State::Attacking:
					anim.currentAnimation = AnimationType::Attacking;
					break;
				}
			}

			if (anim.hasAnimation(anim.currentAnimation)) {

				// Restart animation if changed
				if (anim.previousAnimation != anim.currentAnimation) {
					anim.startAnimation(anim.currentAnimation);
				}

				anim.stepAnimation(anim.currentAnimation, deltaTime);

				const auto currentFrame = anim.currentFrame(anim.currentAnimation);
				texture = currentFrame.texture;
				TileSize = currentFrame.tileSize;
				TileMapPosition = currentFrame.tileMapPosition;
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems
