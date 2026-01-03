#include "HealthSystem.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
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
		const auto &[damage] = view.get<PendingDamageComponent>(entity);

		if (isInvulnerable(health) || damage == 0) {
			registry.remove<PendingDamageComponent>(entity);
			continue;
		}

		health.health -= damage;
		health.invulnerabilityTimer.reset();

		if (health.health <= 0) {
			health.health = 0;
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
