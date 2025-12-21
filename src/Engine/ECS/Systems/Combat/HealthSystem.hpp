#pragma once
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct HealthSystem {
	static void update(Registry &registry, const float deltaTime)
	{
		for (auto entityView = registry.view<HealthComponent>(); const auto entity : entityView) {
			if (auto &health = registry.get<HealthComponent>(entity); !health.invulnerabilityTimer.isTimeout()) {
				health.invulnerabilityTimer.step(std::chrono::duration<float>(deltaTime), false);
			}
		}

		for (auto view = registry.view<HealthComponent, PendingDamageComponent, StateComponent>();
		     const auto entity : view) {
			auto &health = view.get<HealthComponent>(entity);
			const auto &[damage] = view.get<PendingDamageComponent>(entity);

			if (!health.invulnerabilityTimer.isTimeout() || damage == 0) {
				registry.remove<PendingDamageComponent>(entity);
				continue;
			}

			health.health -= damage;
			health.invulnerabilityTimer.reset();
			auto &state = registry.get<StateComponent>(entity);

			if (health.health <= 0) {
				health.health = 0;
				if (registry.has<AnimationComponent>(entity)) {
					if (auto &animation = registry.get<AnimationComponent>(entity);
					    animation.hasAnimation(AnimationType::Dead)) {
						state.state = State::Dead; // TODO: handle death when resolving issue
						                           // https://github.com/tobiasbeiser/coredump-castle/issues/67
					}
				}
			} else {
				if (registry.has<AnimationComponent>(entity)) {
					if (auto &anim = registry.get<AnimationComponent>(entity);
					    anim.hasAnimation(AnimationType::Hurting)) {
						state.state = State::Hurting;
					}
				}
			}
			registry.remove<PendingDamageComponent>(entity);
		}
	}
};
} // namespace GameEngine::ECS::Systems