#include "HitDetectionSystem.hpp"

#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/MeleeAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
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

bool isHostile(const Registry &registry, const Entity attacker, const Entity victim)
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

void resolveAttacks(Registry &registry, const EnemyHitCallback &onHit)
{
	auto attackerView = registry.view<AttackComponent, HitBoxComponent, PositionComponent, HurtBoxComponent,
	                                   RenderComponent>();
	auto victimView = registry.view<HurtBoxComponent, PositionComponent, HealthComponent>();

	for (const auto attacker : attackerView) {
		const auto &attackComponent = attackerView.get<AttackComponent>(attacker);
		const auto &attackerHitBox = attackerView.get<HitBoxComponent>(attacker);
		const auto &[position, previousPosition] = attackerView.get<PositionComponent>(attacker);
		const auto &[attackerHurtBoxRect] = attackerView.get<HurtBoxComponent>(attacker);
		const auto facingLeft = attackerView.get<RenderComponent>(attacker).isFacingLeft();
		const auto &weaponHitBox = registry.tryGet<MeleeAttackComponent>(attacker);

		SDL_FRect attackerReferenceBox = attackerHitBox.bodyHitBox;
		if (const auto sprite = registry.tryGet<SpriteComponent>(attacker)) {
			const auto &tileSize = sprite->get().tileSize;
			attackerReferenceBox = {0.0f, 0.0f, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
		} else if (const auto collision = registry.tryGet<CollisionComponent>(attacker)) {
			attackerReferenceBox = collision->get().collisionBox;
		}

		const auto attackerTransform = registry.tryGet<TransformComponent>(attacker);
		const auto attackerScale = attackerTransform ? &attackerTransform->get().scale : nullptr;

		const bool isAttackingState = AttackSystem::isAttacking(attackComponent);
		bool isWeaponActive = isAttackingState && attackComponent.isDamageActive && weaponHitBox;

		SDL_FRect attackerWeaponRect = {0, 0, 0, 0};
		if (isWeaponActive) {
			SDL_FRect localHurtBox = attackerHurtBoxRect;
			if (facingLeft) {
				localHurtBox.x =
				    attackerReferenceBox.x + attackerReferenceBox.w - (attackerHurtBoxRect.x + attackerHurtBoxRect.w);
			}
			attackerWeaponRect = Utils::HitBoxUtils::getWeaponHitBoxRect(position, weaponHitBox->get().weaponHitBox,
			                                                             localHurtBox, attackerScale, facingLeft);
		}

		SDL_FRect attackerBodyRect = Utils::HitBoxUtils::getHitBoxFacing(
		    position, attackerHitBox.bodyHitBox, attackerReferenceBox, attackerScale, facingLeft);
		for (const auto victim : victimView) {
			if (!isValidVictim(registry, attacker, victim)) {
				continue;
			}
			const auto &[victimPos, victimPPos] = victimView.get<PositionComponent>(victim);
			const auto &[victimHurtBox] = victimView.get<HurtBoxComponent>(victim);
			const auto victimTransform = registry.tryGet<TransformComponent>(victim);
			const auto victimScale = victimTransform ? &victimTransform->get().scale : nullptr;
			SDL_FRect victimReferenceBox = victimHurtBox;
			if (const auto sprite = registry.tryGet<SpriteComponent>(victim)) {
				const auto &tileSize = sprite->get().tileSize;
				victimReferenceBox = {0.0f, 0.0f, static_cast<float>(tileSize.x), static_cast<float>(tileSize.y)};
			} else if (const auto collision = registry.tryGet<CollisionComponent>(victim)) {
				victimReferenceBox = collision->get().collisionBox;
			}
			bool victimFacingLeft = false;
			if (const auto victimRender = registry.tryGet<RenderComponent>(victim)) {
				victimFacingLeft = victimRender->get().isFacingLeft();
			}
			SDL_FRect victimHurtBoxRect = Utils::HitBoxUtils::getHitBoxFacing(
			    victimPos, victimHurtBox, victimReferenceBox, victimScale, victimFacingLeft);

			bool isInvulnerable = false;
			if (const auto health = registry.tryGet<HealthComponent>(victim)) {
				isInvulnerable = HealthSystem::isInvulnerable(health->get());
			}

			bool hit = false;
			bool bodyHit = false;

			if (isWeaponActive && SDL_HasRectIntersectionFloat(&attackerWeaponRect, &victimHurtBoxRect)) {
				if (!isInvulnerable) {
					hit = true;
					onHit(attacker, victim);
				}
			}

			if (!hit && registry.has<EnemyTag>(attacker)
			    && SDL_HasRectIntersectionFloat(&attackerBodyRect, &victimHurtBoxRect)) {
				bodyHit = true;
				if (!isInvulnerable) {
					hit = true;
				}
			}

			if (bodyHit && isInvulnerable) {
				continue;
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
	auto projectileView = registry.view<ProjectileTag, ProjectileComponent, PositionComponent, HitBoxComponent>();
	auto victimView = registry.view<HurtBoxComponent, PositionComponent, HealthComponent>();

	for (const auto projectile : projectileView) {
		auto &[position, previousPosition] = projectileView.get<PositionComponent>(projectile);
		auto &projectileHitBox = projectileView.get<HitBoxComponent>(projectile).bodyHitBox;

		const auto transform = registry.tryGet<TransformComponent>(projectile);
		const auto scale = transform ? &transform->get().scale : nullptr;
		
		// Determine if projectile is flying left based on velocity
		bool facingLeft = false;
		if (const auto velocity = registry.tryGet<VelocityComponent>(projectile)) {
			facingLeft = velocity->get().velocity.x < 0.0f;
		}
		
		// Use facing-aware hitbox calculation so hitbox stays at the tip
		SDL_FRect referenceBox = projectileHitBox; // Use hitbox as reference for flipping
		SDL_FRect projectileHurtBoxRect = Utils::HitBoxUtils::getHitBoxFacing(
		    position, projectileHitBox, referenceBox, scale, facingLeft);

		for (const auto victim : victimView) {
			if (!isValidVictim(registry, projectile, victim)) {
				continue;
			}

			auto &victimPos = victimView.get<PositionComponent>(victim);
			auto &victimHurtBox = victimView.get<HurtBoxComponent>(victim).hurtBox;

			auto victimTransform = registry.tryGet<TransformComponent>(victim);
			const auto victimScale = victimTransform ? &victimTransform->get().scale : nullptr;

			SDL_FRect victimHurtBoxRect = Utils::HitBoxUtils::getHitBox(victimPos.position, victimHurtBox, victimScale);

			if (registry.has<EnemyTag>(projectile)
			    && SDL_HasRectIntersectionFloat(&projectileHurtBoxRect, &victimHurtBoxRect)) {
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
