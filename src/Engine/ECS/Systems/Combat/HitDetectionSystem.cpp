#include "HitDetectionSystem.hpp"

#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
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

constexpr int defaultDamage = 10;

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

bool isValidVictim(Registry &registry, const Entity attacker, const Entity victim)
{
	if (victim == attacker) {
		return false;
	}

	if (!isHostile(registry, attacker, victim)) {
		return false;
	}

	if (registry.has<StateComponent>(victim)) {
		if (registry.get<StateComponent>(victim).state == State::Dead) {
			return false;
		}
	}

	const auto &victimHealth = registry.get<HealthComponent>(victim);
	if (HealthSystem::isDead(victimHealth)) {
		return false;
	}
	return true;
}

void applyDamage(Registry &registry, const Entity victim, const int damage = defaultDamage)
{
	if (!registry.has<PendingDamageComponent>(victim)) {
		registry.registerComponent(victim, PendingDamageComponent{damage});
	}
}

bool checkCollision(const SDL_FRect &rect1, const SDL_FRect &rect2)
{
	return SDL_HasRectIntersectionFloat(&rect1, &rect2);
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
			if (!isValidVictim(registry, attacker, victim)) {
				continue;
			}

			SDL_FRect victimHurtBoxRect = Utils::HitBoxUtils::getHurtBoxRect(registry, victim);
			bool hit = false;

			if (isAttacking && checkCollision(attackerWeaponRect, victimHurtBoxRect)) {
				hit = true;
				onHit(attacker, victim);
			}

			if (!hit && registry.has<EnemyTag>(attacker) && checkCollision(attackerBodyRect, victimHurtBoxRect)) {
				hit = true;
			}

			if (hit) {
				applyDamage(registry, victim);
			}
		}
	}
}

void resolveProjectileHits(Registry &registry)
{
	auto projectileView = registry.view<EnemyTag, ProjectileComponent>();
	auto victimView = registry.view<HurtBoxComponent, PositionComponent, HealthComponent>();

	for (const auto attacker : projectileView) {
		SDL_FRect projectileHurtBox = Utils::HitBoxUtils::getHurtBoxRect(registry, attacker);

		for (const auto victim : victimView) {
			if (!isValidVictim(registry, attacker, victim)) {
				continue;
			}

			SDL_FRect victimHurtBoxRect = Utils::HitBoxUtils::getHurtBoxRect(registry, victim);

			if (registry.has<EnemyTag>(attacker) && checkCollision(projectileHurtBox, victimHurtBoxRect)) {
				applyDamage(registry, victim);
			}
		}
	}
}

} // anonymous namespace

void update(Registry &registry, const EnemyHitCallback &onHit)
{
	PROFILE_SCOPE("HitDetectionSystem");
	resolveAttacks(registry, onHit);
	resolveProjectileHits(registry);
}

} // namespace GameEngine::ECS::Systems::HitDetectionSystem
