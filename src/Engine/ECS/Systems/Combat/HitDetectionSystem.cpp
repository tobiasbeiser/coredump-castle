#include "HitDetectionSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
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
#include "Engine/ECS/View.hpp"

#include <glm/vec2.hpp>

namespace GameEngine::ECS::Systems::HitDetectionSystem {

namespace {

bool isHostile(Registry &registry, Entity attacker, Entity victim)
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

SDL_FRect getHitBoxRect(Registry &registry, Entity entity)
{
	if (!registry.has<HitBoxComponent>(entity) || !registry.has<PositionComponent>(entity)) {
		return {0, 0, 0, 0};
	}

	const auto &[hitbox, isBodyHitBox] = registry.get<HitBoxComponent>(entity);
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

	SDL_FRect result;

	if (isBodyHitBox) {
		result = {position.x + hurtBox.x * scale.x, position.y + hurtBox.y * scale.y, hurtBox.w * scale.x,
		          hurtBox.h * scale.y};
	} else {
		result = {0, 0, hitbox.w * scale.x, hitbox.h * scale.y};
		if (facingLeft) {
			result.x = position.x + hurtBox.x * scale.x - hitbox.w * scale.x;
		} else {
			result.x = position.x + hurtBox.x * scale.x + hurtBox.w * scale.x;
		}
		result.y = position.y + hurtBox.y * scale.y;
	}

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

void resolveAttacks(Registry &registry)
{
	auto attackerView = registry.view<AttackComponent, HitBoxComponent, PositionComponent>();
	auto victimView = registry.view<HurtBoxComponent, PositionComponent, StateComponent>();

	for (const auto attacker : attackerView) {
		const auto &attackComponent = registry.get<AttackComponent>(attacker);

		if (const auto &[attackerHitBox, attackerIsBodyHitBox] = registry.get<HitBoxComponent>(attacker);
		    !attackerIsBodyHitBox && !AttackSystem::isAttacking(attackComponent)) {
			continue;
		}

		SDL_FRect attackerHitBoxRect = getHitBoxRect(registry, attacker);

		for (const auto victim : victimView) {
			if (victim == attacker) {
				continue;
			}

			if (!isHostile(registry, attacker, victim)) {
				continue;
			}

			auto &[victimState] = registry.get<StateComponent>(victim);
			if (victimState == State::Dead) {
				continue;
			}

			SDL_FRect victimHurtBoxRect = getHurtBoxRect(registry, victim);

			if (SDL_HasRectIntersectionFloat(&attackerHitBoxRect, &victimHurtBoxRect)) {
				if (!registry.has<PendingDamageComponent>(victim)) {
					registry.registerComponent(victim, PendingDamageComponent{10});
				}

				victimState = State::Hurting;
				break;
			}
		}
	}
}

} // anonymous namespace

void update(Registry &registry) { resolveAttacks(registry); }

} // namespace GameEngine::ECS::Systems::HitDetectionSystem
