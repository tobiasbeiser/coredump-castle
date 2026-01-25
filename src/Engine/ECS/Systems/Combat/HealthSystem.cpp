#include "HealthSystem.hpp"

#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PoiseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/PendingRespawnComponent.hpp"
#include "Engine/ECS/Components/Render/VignetteComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

#include <SDL3/SDL_log.h>

namespace GameEngine::ECS::Systems::HealthSystem {

namespace {
bool shouldIgnoreDamage(const Registry &registry, const Entity entity, const HealthComponent &health, const int damage,
                        const DamageSource damageSource)
{
	if (registry.has<PlayerTag>(entity) && DebugSystem::instance().getCheats().godMode) {
		return true;
	}
	if ((isInvulnerable(health) || damage == 0) && damageSource != DamageSource::Hazard) {
		return true;
	}
	return false;
}

void applyDamage(HealthComponent &health, const int damage)
{
	health.health = std::max(0, health.health - damage);
	health.invulnerabilityTimer.reset();
}

void handleHazardRespawn(Registry &registry, const Entity entity, const HealthComponent &health,
                         const DamageSource damageSource)
{
	if (damageSource == DamageSource::Hazard && health.health > 0) {
		if (registry.has<LastGroundedPositionComponent>(entity) && registry.has<PlayerTag>(entity)) {
			registry.registerComponent<PendingRespawnComponent>(entity, {});
		}
	}
}

bool checkStagger(Registry &registry, const Entity entity, const int damage)
{
	auto poiseOpt = registry.tryGet<PoiseComponent>(entity);
	if (!poiseOpt) {
		return true;
	}

	auto &poise = poiseOpt->get();
	poise.currentPoise += static_cast<float>(damage);
	poise.recoveryTimer.reset();

	if (poise.currentPoise >= poise.maxPoise) {
		poise.currentPoise = 0.0f;
		return true;
	}
	return false;
}

void handleFlinchEffect(Registry &registry, const Entity entity, HealthComponent &health, const bool isStaggered)
{
	health.invulnerabilityTimer.reset();

	if (isStaggered) {
		if (const auto state = registry.tryGet<StateComponent>(entity)) {
			state->get().state = State::Hurting;
		}
		if (const auto chase = registry.tryGet<ChaseComponent>(entity)) {
			chase->get().reactionDelay.reset();
		}
	} else {
		// Short invulnerability for someone who "tanked" hit else to OP
		const auto length = health.invulnerabilityTimer.getLength();
		if (constexpr auto shortDuration = 200ms; length > shortDuration) {
			health.invulnerabilityTimer.step(length - shortDuration, false);
		}
	}
}

void applyPostDamageEffects(Registry &registry, const Entity entity)
{
	if (registry.has<PlayerTag>(entity)) {
		registry.registerComponent<VignetteComponent>(entity, {});
	}
}

} // namespace

void update(Registry &registry, const float deltaTime)
{
	PROFILE_SCOPE("HealthSystem");

	for (auto entityView = registry.view<HealthComponent>(); const auto entity : entityView) {
		if (auto &health = entityView.get<HealthComponent>(entity); isInvulnerable(health)) {
			health.invulnerabilityTimer.step(std::chrono::duration<float>(deltaTime), false);
		}
	}

	for (auto view = registry.view<HealthComponent, PendingDamageComponent>().exclude<PendingRespawnComponent>();
	     const auto entity : view) {
		auto &health = view.get<HealthComponent>(entity);
		const auto &[damage, damageSource] = view.get<PendingDamageComponent>(entity);

		if (shouldIgnoreDamage(registry, entity, health, damage, damageSource)) {
			registry.remove<PendingDamageComponent>(entity);
			continue;
		}

		applyDamage(health, damage);
		handleHazardRespawn(registry, entity, health, damageSource);

		const bool isStaggered = checkStagger(registry, entity, damage);
		handleFlinchEffect(registry, entity, health, isStaggered);
		applyPostDamageEffects(registry, entity);

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
