#include "SoundSystem.hpp"

#include "Engine/Audio/AudioManager.hpp"
#include "Engine/ECS/Components/Core/SoundComponent.hpp"
#include "Engine/ECS/View.hpp"

void GameEngine::ECS::Systems::SoundSystem::update(Registry &registry, AudioManager &audioManager,
                                                   const float deltaTime)
{
	PROFILE_SCOPE("SoundSystem");

	for (auto view = registry.view<AnimationComponent, SoundComponent>(); const auto entity : view) {
		auto &anim = view.get<AnimationComponent>(entity);
		auto &[sounds, lastAnimation, stepTimer] = view.get<SoundComponent>(entity);

		AnimationType current = anim.getAnimation();

		if (current != lastAnimation) {
			if (sounds.contains(current)) {
				audioManager.playGroup(sounds[current]);
			}

			stepTimer.reset();
			lastAnimation = current;
		}

		if (current == AnimationType::Walking) {
			stepTimer.step(std::chrono::duration<float>(deltaTime), true);
			if (stepTimer.isTimeout()) {
				if (sounds.contains(AnimationType::Walking)) {
					audioManager.playGroup(sounds[AnimationType::Walking]);
				}
				stepTimer.reset();
			}
		}
	}
}