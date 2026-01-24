#include "HitDetectionSystem.hpp"

#include "Engine/Debug/DebugSystem.hpp"
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
#include "Engine/ECS/Components/Tags/ProjectileTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Combat/HealthSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"

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

bool isValidVictim(Registry &registry, const Entity attacker, const Entity victim)
{
	if (victim == attacker) {
		return false;
	}

	if (!isHostile(registry, attacker, victim)) {
		return false;
	}

	if (const auto state = registry.tryGet<StateComponent>(victim)) {
		if (state->get().state == State::Dead) {
			return false;
		}
	}

	if (const auto victimHealth = registry.tryGet<HealthComponent>(victim);
	    victimHealth && HealthSystem::isDead(victimHealth->get())) {
		return false;
	}
	return true;
}

void applyDamage(Registry &registry, const Entity victim, const int damage = 10)
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
	auto attackerView =
	    registry.view<AttackComponent, HitBoxComponent, PositionComponent, HurtBoxComponent, RenderComponent>();
	auto victimView = registry.view<HurtBoxComponent, PositionComponent, HealthComponent>();

	for (const auto attacker : attackerView) {
		const auto &attackComponent = attackerView.get<AttackComponent>(attacker);
		const auto &attackerHitBox = attackerView.get<HitBoxComponent>(attacker);
		const auto &[position, previousPosition] = attackerView.get<PositionComponent>(attacker);
		const auto &[attackerHurtBoxRect] = attackerView.get<HurtBoxComponent>(attacker);
		const auto facingLeft = attackerView.get<RenderComponent>(attacker).flipX;

		const auto attackerTransform = registry.tryGet<TransformComponent>(attacker);
		const glm::vec2 attackerScale = attackerTransform ? attackerTransform->get().scale : glm::vec2{1.0f, 1.0f};

		const bool isAttacking = AttackSystem::isAttacking(attackComponent);

		SDL_FRect attackerWeaponRect = {0, 0, 0, 0};
		if (isAttacking) {
			attackerWeaponRect = Utils::HitBoxUtils::getWeaponHitBoxRect(position, attackerHitBox, attackerHurtBoxRect,
			                                                             attackerScale, facingLeft);
		}

		SDL_FRect attackerBodyRect = Utils::HitBoxUtils::getHitBox(position, attackerHurtBoxRect, attackerScale);
		for (const auto victim : victimView) {
			if (!isValidVictim(registry, attacker, victim)) {
				continue;
			}

			const auto &[victimPos, victimPPos] = victimView.get<PositionComponent>(victim);
			const auto &[victimHurtBox] = victimView.get<HurtBoxComponent>(victim);
			const auto victimTransform = registry.tryGet<TransformComponent>(victim);
			const glm::vec2 victimScale = victimTransform ? victimTransform->get().scale : glm::vec2{1.0f, 1.0f};
			SDL_FRect victimHurtBoxRect = Utils::HitBoxUtils::getHitBox(victimPos, victimHurtBox, victimScale);

			bool hit = false;

			if (isAttacking && checkCollision(attackerWeaponRect, victimHurtBoxRect)) {
				hit = true;
				onHit(attacker, victim);
			}

			if (!hit && registry.has<EnemyTag>(attacker) && checkCollision(attackerBodyRect, victimHurtBoxRect)) {
				hit = true;
			}

			if (hit) {
				int damage = 10;
				if (registry.has<PlayerTag>(attacker) && DebugSystem::instance().getCheats().godMode) {
					damage = INT_MAX;
				}
				applyDamage(registry, victim, damage);
			}
		}
	}
}

void resolveProjectileHits(Registry &registry)
{
	auto projectileView = registry.view<ProjectileTag, ProjectileComponent, PositionComponent, HurtBoxComponent>();
	auto victimView = registry.view<HurtBoxComponent, PositionComponent, HealthComponent>();

	for (const auto projectile : projectileView) {
		auto &[position, previousPosition] = projectileView.get<PositionComponent>(projectile);
		auto &projectileHurtBox = projectileView.get<HurtBoxComponent>(projectile).hurtBox;

		const auto transform = registry.tryGet<TransformComponent>(projectile);
		glm::vec2 scale = transform ? transform->get().scale : glm::vec2{1.0f, 1.0f};

		SDL_FRect projectileHurtBoxRect = Utils::HitBoxUtils::getHitBox(position, projectileHurtBox, scale);

		for (const auto victim : victimView) {
			if (!isValidVictim(registry, projectile, victim)) {
				continue;
			}

			auto &victimPos = victimView.get<PositionComponent>(victim);
			auto &victimHurtBox = victimView.get<HurtBoxComponent>(victim).hurtBox;

			auto victimTransform = registry.tryGet<TransformComponent>(victim);
			glm::vec2 victimScale = victimTransform ? victimTransform->get().scale : glm::vec2{1.0f, 1.0f};

			SDL_FRect victimHurtBoxRect = Utils::HitBoxUtils::getHitBox(victimPos.position, victimHurtBox, victimScale);

			if (registry.has<EnemyTag>(projectile) && checkCollision(projectileHurtBoxRect, victimHurtBoxRect)) {
				applyDamage(registry, victim);
				registry.deleteEntity(projectile);
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
