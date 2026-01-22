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

SDL_FRect getWeaponHitBoxRect(Registry &registry, const Entity entity)
{
	if (!registry.has<HitBoxComponent>(entity) || !registry.has<PositionComponent>(entity)) {
		return {0, 0, 0, 0};
	}

	const auto &[bodyHitBox, weaponHitBox] = registry.get<HitBoxComponent>(entity);
	const auto &[position, previousPosition] = registry.get<PositionComponent>(entity);

	bool facingLeft = false;
	if (registry.has<RenderComponent>(entity)) {
		facingLeft = registry.get<RenderComponent>(entity).flipX;
	}

	glm::vec2 scale = {1.0f, 1.0f};
	if (registry.has<TransformComponent>(entity)) {
		scale = registry.get<TransformComponent>(entity).scale;
	}

	if (!registry.has<HurtBoxComponent>(entity)) {
		return {0, 0, 0, 0};
	}
	const auto &[hurtBox] = registry.get<HurtBoxComponent>(entity);

	SDL_FRect result = {0, 0, weaponHitBox.w * scale.x, weaponHitBox.h * scale.y};
	if (facingLeft) {
		result.x = position.x + hurtBox.x * scale.x - weaponHitBox.w * scale.x;
	} else {
		result.x = position.x + hurtBox.x * scale.x + hurtBox.w * scale.x;
	}
	result.y = position.y + hurtBox.y * scale.y;

	return result;
}

SDL_FRect getHurtBoxRect(Registry &registry, const Entity entity)
{
	if (!registry.has<HurtBoxComponent>(entity) || !registry.has<PositionComponent>(entity)) {
		return {0, 0, 0, 0};
	}

	const auto &[hurtBox] = registry.get<HurtBoxComponent>(entity);
	const auto &[position, previousPosition] = registry.get<PositionComponent>(entity);

	glm::vec2 scale = {1.0f, 1.0f};
	if (registry.has<TransformComponent>(entity)) {
		scale = registry.get<TransformComponent>(entity).scale;
	}

	return {position.x + hurtBox.x * scale.x, position.y + hurtBox.y * scale.y, hurtBox.w * scale.x,
	        hurtBox.h * scale.y};
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
			attackerWeaponRect = getWeaponHitBoxRect(registry, attacker);
		}

		SDL_FRect attackerBodyRect = getHurtBoxRect(registry, attacker);

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

			SDL_FRect victimHurtBoxRect = getHurtBoxRect(registry, victim);
			bool hit = false;

			if (isAttacking) {
				if (SDL_HasRectIntersectionFloat(&attackerWeaponRect, &victimHurtBoxRect)) {
					hit = true;
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
					onHit(attacker, victim);
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
