#include "HealthSystem.hpp"

#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/PendingRespawnComponent.hpp"
#include "Engine/ECS/Components/Render/VignetteComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

#include <SDL3/SDL_log.h>

namespace GameEngine::ECS::Systems::HealthSystem {

void update(Registry &registry, const float deltaTime)
{
	PROFILE_SCOPE("AttackSystem");

	for (auto entityView = registry.view<HealthComponent>(); const auto entity : entityView) {
		if (auto &health = entityView.get<HealthComponent>(entity); isInvulnerable(health)) {
			health.invulnerabilityTimer.step(std::chrono::duration<float>(deltaTime), false);
		}
	}

	for (auto view = registry.view<HealthComponent, PendingDamageComponent>().exclude<PendingRespawnComponent>();
	     const auto entity : view) {
		auto &health = view.get<HealthComponent>(entity);
		const auto &[damage, damageSource] = view.get<PendingDamageComponent>(entity);

		if (registry.has<PlayerTag>(entity) && DebugSystem::instance().getCheats().godMode) {
			registry.remove<PendingDamageComponent>(entity);
			continue;
		}

		if ((isInvulnerable(health) || damage == 0) && damageSource != DamageSource::Hazard) {
			registry.remove<PendingDamageComponent>(entity);
			continue;
		}

		health.health -= damage;
		health.invulnerabilityTimer.reset();

		if (health.health <= 0) {
			health.health = 0;
		} else if (damageSource == DamageSource::Hazard) {
			if (registry.has<LastGroundedPositionComponent>(entity) && registry.has<PlayerTag>(entity)) {
				registry.registerComponent<PendingRespawnComponent>(entity, {});
			}
		}

		bool isStaggered = true;
		if (health.maxPoise > 0.0f) {
			health.currentPoise += static_cast<float>(damage);
			if (health.currentPoise >= health.maxPoise) {
				health.currentPoise = 0.0f;
				isStaggered = true;
			} else {
				isStaggered = false;
			}
		}

		health.invulnerabilityTimer.reset();

		if (isStaggered) {
			if (const auto state = registry.tryGet<StateComponent>(entity)) {
				state->get().state = State::Hurting;
			}
			if (const auto chase = registry.tryGet<ChaseComponent>(entity)) {
				chase->get().reactionDelay.reset();
			}
		} else {
			// Poise held. Grant only short invulnerability (200ms) without changing the timer's permanent length.
			// We do this by advancing the timer so it is closer to timeout.
			const auto length = health.invulnerabilityTimer.getLength();
			const auto shortDuration = 200ms;
			if (length > shortDuration) {
				health.invulnerabilityTimer.step(length - shortDuration, false);
			}
		}

		registry.remove<PendingDamageComponent>(entity);
		if (registry.has<PlayerTag>(entity)) {
			registry.registerComponent<VignetteComponent>(entity, {});
		}
	}
}

bool isHurting(const HealthComponent &health)
{
	return isInvulnerable(health) && !isDead(health);
}

bool isDead(const HealthComponent &health)
{
	return health.health <= 0;
}

bool isInvulnerable(const HealthComponent &health)
{
	return !health.invulnerabilityTimer.isTimeout();
}

} // namespace GameEngine::ECS::Systems::HealthSystem
