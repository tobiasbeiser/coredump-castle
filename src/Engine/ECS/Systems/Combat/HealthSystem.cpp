#include "HealthSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/PendingRespawnComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems::HealthSystem {

void update(Registry &registry, const float deltaTime)
{
	for (auto entityView = registry.view<HealthComponent>(); const auto entity : entityView) {
		if (auto &health = registry.get<HealthComponent>(entity); isInvulnerable(health)) {
			health.invulnerabilityTimer.step(std::chrono::duration<float>(deltaTime), false);
		}
	}

	for (auto view = registry.view<HealthComponent, PendingDamageComponent>(); const auto entity : view) {
		auto &health = view.get<HealthComponent>(entity);
		const auto &[damage, damageSource] = view.get<PendingDamageComponent>(entity);

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

		if (registry.has<ChaseComponent>(entity)) {
			auto &chase = registry.get<ChaseComponent>(entity);
			chase.reactionDelay.reset();
		}

		if (damageSource == DamageSource::Hazard) {
			if (registry.has<LastGroundedPositionComponent>(entity) && registry.has<TransformComponent>(entity)) {
				const auto &[lastGroundedPosition] = registry.get<LastGroundedPositionComponent>(entity);
				auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
				position = lastGroundedPosition;
			}
		}

		registry.remove<PendingDamageComponent>(entity);
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
