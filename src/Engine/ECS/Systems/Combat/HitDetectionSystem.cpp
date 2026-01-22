#include "HitDetectionSystem.hpp"

#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Combat/HealthSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"

#include <glm/vec2.hpp>

namespace GameEngine::ECS::Systems::HitDetectionSystem {

namespace {

bool isHostile(Registry &registry, const Entity attacker, const Entity victim)
{
	const auto attackerIsPlayer = registry.has<PlayerTag>(attacker);
	const auto attackerIsEnemy = registry.has<EnemyTag>(attacker);

	const auto victimIsPlayer = registry.has<PlayerTag>(victim);
	const auto victimIsEnemy = registry.has<EnemyTag>(victim);

	if (attackerIsPlayer && victimIsEnemy)
		return true;

	if (attackerIsEnemy && victimIsPlayer)
		return true;

	return false;
}

void resolveAttacks(Registry &registry, const EnemyHitCallback &onHit)
{
	auto attackerView = registry.view<AttackComponent, HitBoxComponent, PositionComponent>();
	auto victimView = registry.view<HurtBoxComponent, PositionComponent, HealthComponent>();

	for (const auto attacker : attackerView) {
		const auto &attackComponent = registry.get<AttackComponent>(attacker);
		const auto &[attackerBodyHitBox, attackerWeaponHitBox] = registry.get<HitBoxComponent>(attacker);

		const bool isAttacking = AttackSystem::isAttacking(attackComponent);

		SDL_FRect attackerWeaponRect = {0, 0, 0, 0};
		if (isAttacking) {
			attackerWeaponRect = Utils::HitBoxUtils::getWeaponHitBoxRect(registry, attacker);
		}

		SDL_FRect attackerBodyRect = Utils::HitBoxUtils::getHurtBoxRect(registry, attacker);

		for (const auto victim : victimView) {
			if (victim == attacker) {
				continue;
			}

			if (!isHostile(registry, attacker, victim)) {
				continue;
			}

			const auto &victimHealth = registry.get<HealthComponent>(victim);
			if (HealthSystem::isDead(victimHealth) || HealthSystem::isInvulnerable(victimHealth)) {
				continue;
			}

			SDL_FRect victimHurtBoxRect = Utils::HitBoxUtils::getHurtBoxRect(registry, victim);
			bool hit = false;

			if (isAttacking) {
				if (SDL_HasRectIntersectionFloat(&attackerWeaponRect, &victimHurtBoxRect)) {
					hit = true;
					onHit(attacker, victim);
				}
			}

			// always body damage on for enemies, but not for the player
			if (!hit && registry.has<EnemyTag>(attacker)) {
				if (SDL_HasRectIntersectionFloat(&attackerBodyRect, &victimHurtBoxRect)) {
					hit = true;
				}
			}

			if (hit) {
				if (!registry.has<PendingDamageComponent>(victim)) {
					registry.registerComponent<PendingDamageComponent>(victim, {10});
				}
			}
		}
	}
}

} // anonymous namespace

void update(Registry &registry, const EnemyHitCallback &onHit)
{
	PROFILE_SCOPE("HitDetectionSystem");

	resolveAttacks(registry, onHit);
}

} // namespace GameEngine::ECS::Systems::HitDetectionSystem
