#include "MeleeActions.hpp"

#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/MeleeAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/ChargeTelegraphComponent.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ChargeSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"
#include "Engine/Utils/Random.hpp"
#include <math.h>

#include <cmath>

namespace GameEngine::ECS {

namespace Actions {

void MeleeChaseAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	Systems::ChaseSystem::updateEntity(registry, entity, deltaTime);
}

void ExecuteAttackAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (auto input = registry.tryGet<InputComponent>(entity)) {
		input->get().attackPressed = true;
	}

	if (auto attack = registry.tryGet<AttackComponent>(entity)) {
		attack->get().activeWindUpLimit =
		    Utils::Random::getFloat(attack->get().windUpLimit * 0.5f, attack->get().windUpLimit);
	}

	auto meleeAttack = registry.tryGet<MeleeAttackComponent>(entity);
	auto enemy = getEntityInfo(registry, entity);
	auto player = getPlayerInfo(registry);

	if (meleeAttack && enemy && player) {
		const float enemyBottom = enemy->hitBoxRect.y + enemy->hitBoxRect.h;
		const float playerBottom = player->hitBoxRect.y + player->hitBoxRect.h;

		if (std::abs(enemyBottom - playerBottom) > 10.0f) {
			meleeAttack->get().weaponHitBox.h = enemy->hitBoxRect.h / enemy->scale.y;
			meleeAttack->get().weaponHitBox.y =
			    (enemy->hitBoxRect.h * 0.5f) / enemy->scale.y - (meleeAttack->get().weaponHitBox.h * 0.5f);
		} else {
			// Player is on the same level - reset to original hitbox
			meleeAttack->get().weaponHitBox = meleeAttack->get().originalWeaponHitBox;
		}
	}
}

void CancelMeleeAttackAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (auto input = registry.tryGet<InputComponent>(entity))
		input->get().attackPressed = false;
	if (auto attack = registry.tryGet<AttackComponent>(entity)) {
		attack->get().attackTimer.reset();
		attack->get().isDamageActive = false;
	}
	if (auto meleeAttack = registry.tryGet<MeleeAttackComponent>(entity)) {
		meleeAttack->get().weaponHitBox = meleeAttack->get().originalWeaponHitBox;
	}
}

void StartChargeAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto chargeOpt = registry.tryGet<ChargeComponent>(entity);
	if (!chargeOpt) {
		return;
	}

	auto &charge = chargeOpt->get();

	charge.chargeTimer.reset();
	charge.chargeTimer.step(std::chrono::duration<float>(0.0f), false);

	charge.chargePauseTimer.reset();

	// Lock direction to player at start of charge
	auto enemy = getEntityInfo(registry, entity);
	auto player = getPlayerInfo(registry);

	if (enemy && player) {
		charge.chargeDirection = (player->center.x > enemy->center.x) ? 1.0f : -1.0f;
	} else {
		charge.chargeDirection = 1.0f;
	}

	charge.chargeCooldown.reset();
}

void ChargeAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	Systems::ChargeSystem::updateEntity(registry, entity, deltaTime);
}

void StartChargeTelegraphAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (!registry.has<ChargeTelegraphComponent>(entity)) {
		registry.registerComponent(entity, ChargeTelegraphComponent{});
	}

	auto &telegraph = registry.get<ChargeTelegraphComponent>(entity);
	telegraph.start();
}

void StopChargeTelegraphAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	if (const auto telegraphOpt = registry.tryGet<ChargeTelegraphComponent>(entity)) {
		telegraphOpt->get().active = false;
	}
}

void FacePlayerDuringWindupAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	const auto attack = registry.tryGet<AttackComponent>(entity);
	if (!attack)
		return;

	if (attack->get().attackTimer.getTime() >= std::chrono::duration<float>(attack->get().activeWindUpLimit)) {
		return;
	}

	FacePlayerAction facePlayer;
	facePlayer.execute(registry, entity, deltaTime);
}

} // namespace Actions

namespace Transitions {

bool MeleeAttackTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto enemyChase = registry.tryGet<ChaseComponent>(entity);
	if (!enemyChase)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return false;

	if (!enemyChase->get().reactionDelay.isTimeout()) {
		return false;
	}
	const auto meleeAttack = registry.tryGet<MeleeAttackComponent>(entity);
	if (!meleeAttack)
		return false;

	const float weaponWidth = meleeAttack->get().weaponHitBox.w * enemy->scale.x;

	float distToPlayer = 0.0f;
	bool isPlayerFullyInside = false;
	if (player->hitBoxRect.x < enemy->hitBoxRect.x + enemy->hitBoxRect.w
	    && player->hitBoxRect.x + player->hitBoxRect.w > enemy->hitBoxRect.x) {
		isPlayerFullyInside = true;
	} else if (player->center.x > enemy->center.x) {
		if (distToPlayer + player->hitBoxRect.w <= weaponWidth) {
			isPlayerFullyInside = true;
		}
	} else {
		distToPlayer = enemy->hitBoxRect.x - (player->hitBoxRect.x + player->hitBoxRect.w);
		if (distToPlayer + player->hitBoxRect.w <= weaponWidth) {
			isPlayerFullyInside = true;
		}
	}

	return isPlayerFullyInside;
}

bool ChargeReadyTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto charge = registry.tryGet<ChargeComponent>(entity);
	if (!charge)
		return false;

	if (!charge->get().chargeCooldown.isTimeout()) {
		return false;
	}

	auto enemy = getEntityInfo(registry, entity);
	auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return false;

	// Only charge if on basically the same floor level (strict vertical check)
	const float enemyBottom = enemy->hitBoxRect.y + enemy->hitBoxRect.h;
	const float playerBottom = player->hitBoxRect.y + player->hitBoxRect.h;

	return std::abs(enemyBottom - playerBottom) < 10.0f;
}

bool ChargeFinishedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto charge = registry.tryGet<ChargeComponent>(entity);
	if (!charge)
		return true;

	return !charge->get().chargeTimer.isRunning();
}

bool PoiseBreakTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	if (auto stateComp = registry.tryGet<StateComponent>(entity)) {
		return stateComp->get().state == State::Hurting;
	}

	return false;
}

bool ChargeTelegraphFinishedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto telegraphOpt = registry.tryGet<ChargeTelegraphComponent>(entity);
	if (!telegraphOpt) {
		return true;
	}

	return telegraphOpt->get().isFinished();
}

} // namespace Transitions

} // namespace GameEngine::ECS
