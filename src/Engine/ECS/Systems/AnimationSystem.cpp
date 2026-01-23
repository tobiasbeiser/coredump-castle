#include "AnimationSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace {
void handleJumping(AnimationComponent &anim)
{
	if (!anim.hasAnimation(AnimationType::JumpStart)) {
		return;
	}
	switch (anim.getAnimation()) {
	case AnimationType::Attacking:
		anim.setAnimation(AnimationType::JumpStart);
		break;
	case AnimationType::JumpStart:
		if (!anim.hasFinished(anim.getAnimation())) {
			anim.setAnimation(AnimationType::JumpStart);
		} else {
			anim.setAnimation(AnimationType::FlyingUp);
		}
		break;
	case AnimationType::FlyingUp:
		anim.setAnimation(AnimationType::FlyingUp);
		break;
	case AnimationType::JumpReload:
		if (!anim.hasFinished(anim.getAnimation())) {
			anim.setAnimation(AnimationType::JumpReload);
		} else {
			anim.setAnimation(AnimationType::FlyingUp);
		}
		break;
	case AnimationType::Falling:
		anim.setAnimation(AnimationType::JumpReload);
		break;
	default:;
	}
}
void handleFalling(AnimationComponent &anim)
{
	if (!anim.hasAnimation(AnimationType::Falling)) {
		return;
	}
	switch (anim.getAnimation()) {
	case AnimationType::Landing:
		anim.setAnimation(AnimationType::JumpReload);
		break;
	case AnimationType::JumpReload:
		if (!anim.hasFinished(anim.getAnimation())) {
			anim.setAnimation(AnimationType::JumpReload);
		} else {
			anim.setAnimation(AnimationType::Falling);
		}
		break;
	case AnimationType::Falling:
		anim.setAnimation(AnimationType::Falling);
		break;
	default:;
	}
	anim.setAnimation(AnimationType::Falling);
}
void handleLanding(AnimationComponent &anim, const AnimationType fallback)
{
	if (anim.hasAnimation(AnimationType::Landing)
	    && (anim.getAnimation() == AnimationType::Falling
	        || (anim.getAnimation() == AnimationType::Landing && !anim.hasFinished(anim.getAnimation())))) {
		anim.setAnimation(AnimationType::Landing);
	} else {
		anim.setAnimation(fallback);
	}
}
} // namespace

void GameEngine::ECS::Systems::AnimationSystem::update(Registry &registry, const float deltaTime)
{
	PROFILE_SCOPE("AnimationSystem");
	for (auto view = registry.view<AnimationComponent, SpriteComponent>(); const auto entity : view) {
		auto &anim = view.get<AnimationComponent>(entity);
		auto &[texture, tileSize, tileMapPosition, alpha] = view.get<SpriteComponent>(entity);

		// Determine current animation from state

		if (auto state = registry.tryGet<StateComponent>(entity); state.has_value()) {
			switch (state->get().state) {
			case State::Idle:
				handleLanding(anim, AnimationType::Idle);
				break;
			case State::Walking:
				handleLanding(anim, AnimationType::Walking);
				break;
			case State::Jumping:
				handleJumping(anim);
				break;
			case State::Falling:
				handleFalling(anim);
				break;
			case State::Attacking:
				if (anim.hasAnimation(AnimationType::Attacking)) {
					anim.setAnimation(AnimationType::Attacking);
				}
				break;
			case State::Hurting:
				if (anim.hasAnimation(AnimationType::Hurting)) {
					anim.setAnimation(AnimationType::Hurting);
				}
				break;
			case State::Dead:
				if (anim.hasAnimation(AnimationType::Dead)) {
					anim.setAnimation(AnimationType::Dead);
				}
				break;
			case State::Interacting:
				if (anim.hasAnimation(AnimationType::Interacting)) {
					anim.setAnimation(AnimationType::Interacting);
				}
				break;
			}
		}

		if (anim.hasAnimation(anim.getAnimation())) {

			anim.stepAnimation(anim.getAnimation(), std::chrono::duration<float>(deltaTime));

			const auto [frameTexture, frameTileSize, frameTileMapPosition] = anim.currentFrame(anim.getAnimation());
			texture = frameTexture;
			tileSize = frameTileSize;
			tileMapPosition = frameTileMapPosition;
		}
	}
}