#include "DeathSystem.hpp"

#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/View.hpp"
#include "HealthSystem.hpp"

void GameEngine::ECS::Systems::DeathSystem::update(Registry &registry)
{
	for (auto view = registry.view<HealthComponent>(); const auto entity : view) {
		auto &health = view.get<HealthComponent>(entity);

		if (HealthSystem::isDead(health)) {
			if (registry.has<AnimationComponent>(entity)) {
				if (auto &anim = registry.get<AnimationComponent>(entity);
				    anim.hasAnimation(AnimationType::Dead) && !anim.hasFinished(AnimationType::Dead)) {
					continue;
				}
			}
			registry.deleteEntity(entity);
		}
	}
}
