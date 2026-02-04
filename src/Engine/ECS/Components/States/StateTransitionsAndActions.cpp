#include "StateTransitionsAndActions.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/MeleeAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PoiseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/ChargeTelegraphComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ChargeSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"
#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
#include "Engine/Utils/Random.hpp"
#include "Game/Factories/ProjectileFactory.hpp"
#include <cmath>
#include <optional>

namespace GameEngine::ECS {

namespace {

struct EntityInfo {
	glm::vec2 position{};
	SDL_FRect rect{};
	glm::vec2 center{};
	glm::vec2 scale{};
};

std::optional<EntityInfo> getEntityInfo(Registry &registry, Entity entity)
{
	auto posOpt = registry.tryGet<PositionComponent>(entity);
	auto hurtBoxOpt = registry.tryGet<HurtBoxComponent>(entity);

	if (!posOpt || !hurtBoxOpt)
		return std::nullopt;

	auto transformOpt = registry.tryGet<TransformComponent>(entity);
	glm::vec2 scale = transformOpt ? transformOpt->get().scale : glm::vec2{1.0f, 1.0f};
	SDL_FRect rect = Utils::HitBoxUtils::getHitBox(posOpt->get().position, hurtBoxOpt->get().hurtBox, &scale);

	return EntityInfo{posOpt->get().position, rect, {rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f}, scale};
}

std::optional<EntityInfo> getPlayerInfo(Registry &registry)
{
	auto playerView = registry.view<PlayerTag, PositionComponent, HurtBoxComponent>();
	for (const auto entity : playerView) {
		return getEntityInfo(registry, entity);
	}
	return std::nullopt;
}

struct ProjectileInfo {
	glm::vec2 size{0.0f, 0.0f};
	glm::vec2 offset{0.0f, 0.0f};
};

ProjectileInfo getProjectileInfo(const ProjectileComponent *projComp)
{
	if (!projComp || projComp->projectileTilesetPath.empty())
		return {};

	const auto &data = ProjectileFactory::getCachedProjectileData(projComp->projectileTilesetPath);
	return {{data.hitbox.w, data.hitbox.h}, {data.hitbox.x, data.hitbox.y}};
}

bool hasLosToAttacker(const EntityInfo &enemy, const EntityInfo &player, const RangedAttackComponent *ranged,
                      const ProjectileComponent *proj)
{
	auto projInfo = getProjectileInfo(proj);

	bool facingLeft = (player.center.x - enemy.position.x) < 0.0f;

	glm::vec2 spawnPos = enemy.position;
	float relativeHurtboxX = enemy.rect.x - enemy.position.x;
	float xOffset = relativeHurtboxX;

	if (ranged) {
		xOffset += facingLeft ? -ranged->projectileOffset.x : ranged->projectileOffset.x;
		spawnPos.y += ranged->projectileOffset.y;
	}
	spawnPos.x += xOffset;

	const glm::vec2 rayStart = spawnPos + projInfo.offset + (projInfo.size * 0.5f);

	auto targetOpt = Utils::DetectionRangeUtils::getTargetWithLineOfSight(rayStart, player.center, player.rect,
	                                                                      player.scale, projInfo.size);

	return targetOpt.has_value();
}

} // namespace

void PatrolAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	Systems::PatrolSystem::updateEntity(registry, entity);
}

void ToggleComponentAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto toggle = [&](auto tag) {
		using T = decltype(tag);
		if (enable && !registry.has<T>(entity)) {
			registry.registerComponent<T>(entity, T{});
		} else if (!enable && registry.has<T>(entity)) {
			registry.remove<T>(entity);
		}
	};

	switch (component) {
	case ComponentType::Charge:
		toggle(ChargeComponent{});
		break;
	case ComponentType::Chase:
		toggle(ChaseComponent{});
		break;
	case ComponentType::Patrol:
		toggle(PatrolComponent{});
		break;
	case ComponentType::Gravity:
		toggle(GravityComponent{});
		break;
	case ComponentType::Collision:
		toggle(CollisionComponent{});
		break;
	}
}

void SetStateComponentAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (auto stateComp = registry.tryGet<StateComponent>(entity)) {
		stateComp->get().state = state;
	}
}

void FacePlayerAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto enemyRender = registry.tryGet<RenderComponent>(entity);
	if (!enemyRender)
		return;

	auto enemy = getEntityInfo(registry, entity);
	auto player = getPlayerInfo(registry);

	if (enemy && player) {
		const bool shouldFaceLeft = player->center.x < enemy->center.x;
		enemyRender->get().setFacingLeft(shouldFaceLeft);
	}
}

void FacePlayerDuringWindupAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto attack = registry.tryGet<AttackComponent>(entity);
	if (!attack)
		return;

	// Only face player if we are in the windup phase
	if (attack->get().attackTimer.getTime() >= std::chrono::duration<float>(attack->get().activeWindUpLimit)) {
		return;
	}

	// Reuse FacePlayer logic
	FacePlayerAction facePlayer;
	facePlayer.execute(registry, entity, deltaTime);
}

void MoveTowardsPlayerAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto enemyPos = registry.tryGet<PositionComponent>(entity);
	auto enemyInput = registry.tryGet<InputComponent>(entity);
	auto enemyRender = registry.tryGet<RenderComponent>(entity);

	if (!enemyPos || !enemyInput)
		return;

	auto player = getPlayerInfo(registry);
	if (!player)
		return;

	float direction = player->center.x > enemyPos->get().position.x ? 1.0f : -1.0f;
	enemyInput->get().horizontalDirection = direction;

	if (enemyRender) {
		enemyRender->get().setFacingLeft(direction < 0);
	}
}

void ExecuteAttackAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (auto input = registry.tryGet<InputComponent>(entity)) {
		input->get().attackPressed = true;
	}

	if (auto attack = registry.tryGet<AttackComponent>(entity)) {
		// Set random windup to vary attack timing
		attack->get().activeWindUpLimit =
		    Utils::Random::getFloat(attack->get().windUpLimit * 0.5f, attack->get().windUpLimit);
	}
}

AnimationType stringToAnimationType(const std::string &animName)
{
	if (animName == "idle")
		return AnimationType::Idle;
	if (animName == "walk" || animName == "walking")
		return AnimationType::Walking;
	if (animName == "jump" || animName == "jumpstart")
		return AnimationType::JumpStart;
	if (animName == "flyingup" || animName == "flying")
		return AnimationType::FlyingUp;
	if (animName == "jumpreload")
		return AnimationType::JumpReload;
	if (animName == "falling" || animName == "fall")
		return AnimationType::Falling;
	if (animName == "landing" || animName == "land")
		return AnimationType::Landing;
	if (animName == "attacking" || animName == "attack")
		return AnimationType::Attacking;
	if (animName == "hurting" || animName == "hurt")
		return AnimationType::Hurting;
	if (animName == "dead" || animName == "dying")
		return AnimationType::Dead;
	if (animName == "charge" || animName == "charging")
		return AnimationType::Charge;
	if (animName == "fire" || animName == "firing" || animName == "shoot")
		return AnimationType::Fire;
	if (animName == "interacting" || animName == "interact")
		return AnimationType::Interacting;

	return AnimationType::None;
}

void TriggerAnimationAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto animComp = registry.tryGet<AnimationComponent>(entity);
	if (!animComp)
		return;

	const AnimationType animType = stringToAnimationType(animationName);
	if (animType != AnimationType::None) {
		animComp->get().setAnimation(animType);
	}
}

void StopAndFacePlayerAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto enemyRender = registry.tryGet<RenderComponent>(entity);
	auto inputOpt = registry.tryGet<InputComponent>(entity);

	if (!enemyRender || !inputOpt)
		return;

	auto enemy = getEntityInfo(registry, entity);
	auto player = getPlayerInfo(registry);

	if (enemy && player) {
		enemyRender->get().setFacingLeft(player->center.x < enemy->center.x);
		inputOpt->get().horizontalDirection = 0.0f;
	}
}

void TriggerRangedAttackAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto inputOpt = registry.tryGet<InputComponent>(entity);
	auto attackOpt = registry.tryGet<AttackComponent>(entity);
	auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);

	if (!inputOpt || !attackOpt || !rangedOpt)
		return;

	auto &input = inputOpt->get();
	auto &attack = attackOpt->get();
	auto &ranged = rangedOpt->get();

	// Check dead players
	auto playerView = registry.view<PlayerTag, StateComponent>();
	for (const auto playerEntity : playerView) {
		if (playerView.get<StateComponent>(playerEntity).state == State::Dead) {
			input.attackPressed = false;
			attack.attackTimer.reset();
			ranged.targetSnapshot = glm::vec2(0.0f);
			ranged.projectileSpawned = false;
			return;
		}
		break;
	}

	// No valid target
	if (ranged.targetSnapshot == glm::vec2(0.0f)) {
		input.attackPressed = false;
		attack.attackTimer.reset();
		return;
	}

	if (!Systems::AttackSystem::isAttacking(attack)) {
		input.attackPressed = true;
		attack.activeWindUpLimit = Utils::Random::getFloat(attack.windUpLimit * 0.5f, attack.windUpLimit);
	}
}

void CancelRangedAttackAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (auto input = registry.tryGet<InputComponent>(entity))
		input->get().attackPressed = false;
	if (auto attack = registry.tryGet<AttackComponent>(entity))
		attack->get().attackTimer.reset();
	if (auto ranged = registry.tryGet<RangedAttackComponent>(entity)) {
		ranged->get().targetSnapshot = glm::vec2(0.0f);
		ranged->get().projectileSpawned = false;
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
}

void AimAtPlayerWithLOSAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return;

	auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	if (!rangedOpt)
		return;

	auto projOpt = registry.tryGet<ProjectileComponent>(entity);
	auto renderOpt = registry.tryGet<RenderComponent>(entity);
	if (!renderOpt)
		return;

	auto player = getPlayerInfo(registry);
	if (!player) {
		rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
		return;
	}

	auto projInfo = getProjectileInfo(projOpt ? &projOpt->get() : nullptr);
	bool facingLeft = renderOpt->get().isFacingLeft();

	glm::vec2 spawnPos = enemy->position;
	float relativeHurtboxX = enemy->rect.x - enemy->position.x;
	float xOffset = relativeHurtboxX;

	xOffset += facingLeft ? -rangedOpt->get().projectileOffset.x : rangedOpt->get().projectileOffset.x;
	spawnPos.y += rangedOpt->get().projectileOffset.y;
	spawnPos.x += xOffset;

	const glm::vec2 rayStart = spawnPos + projInfo.offset + (projInfo.size * 0.5f);

	auto targetOpt = Utils::DetectionRangeUtils::getTargetWithLineOfSight(rayStart, player->center, player->rect,
	                                                                      player->scale, projInfo.size);

	if (targetOpt) {
		rangedOpt->get().targetSnapshot = *targetOpt;
	} else {
		rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
	}
}

void MeleeChaseAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	Systems::ChaseSystem::updateEntity(registry, entity, deltaTime);
}

void StartChargeAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto chargeOpt = registry.tryGet<ChargeComponent>(entity);
	if (!chargeOpt) {
		registry.registerComponent(entity, ChargeComponent{});
		chargeOpt = registry.tryGet<ChargeComponent>(entity);
	}

	auto &charge = chargeOpt->get();

	charge.chargeTimer.reset();
	charge.chargeTimer.step(std::chrono::duration<float>(0.0f), false);

	charge.chargePauseTimer.reset();

	// Lock direction to player at start of charge
	const auto posOpt = registry.tryGet<PositionComponent>(entity);
	auto playerView = registry.view<PlayerTag, PositionComponent>();
	if (posOpt) {
		for (const auto playerEntity : playerView) {
			const auto &playerPos = playerView.get<PositionComponent>(playerEntity);
			charge.chargeDirection = (playerPos.position.x > posOpt->get().position.x) ? 1.0f : -1.0f;
			break; // Found a player (single player game assumption for now)
		}
	} else {
		charge.chargeDirection = 1.0f; // Default
	}

	// Reset cooldown timer in ChaseComponent
	if (auto chaseOpt = registry.tryGet<ChaseComponent>(entity)) {
		chaseOpt->get().chargeCooldown.reset();
	}
}

bool ChargeReadyTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto chaseOpt = registry.tryGet<ChaseComponent>(entity);
	if (!chaseOpt)
		return false;

	return chaseOpt->get().chargeCooldown.isTimeout();
}

void ChargeAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	Systems::ChargeSystem::updateEntity(registry, entity, deltaTime);
}

bool ChargeFinishedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	return !registry.has<ChargeComponent>(entity);
}

void StartChargeTelegraphAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	// Register component if not present
	if (!registry.has<ChargeTelegraphComponent>(entity)) {
		registry.registerComponent(entity, ChargeTelegraphComponent{});
	}

	auto &telegraph = registry.get<ChargeTelegraphComponent>(entity);
	telegraph.start();
}

bool ChargeTelegraphFinishedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto telegraphOpt = registry.tryGet<ChargeTelegraphComponent>(entity);
	if (!telegraphOpt) {
		return true; // No telegraph component means we can transition
	}

	return telegraphOpt->get().isFinished();
}

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

	// Check if reaction delay has passed
	if (!enemyChase->get().reactionDelay.isTimeout()) {
		return false;
	}

	// Get weapon range from MeleeAttackComponent
	auto meleeAttack = registry.tryGet<MeleeAttackComponent>(entity);
	if (!meleeAttack)
		return false;

	// Calculate the gap between enemy body edge and player body edge
	const float xDistance = std::abs(player->center.x - enemy->center.x);
	const float combinedHalfWidths = (player->rect.w * 0.5f) + (enemy->rect.w * 0.5f);
	const float edgeGap = xDistance - combinedHalfWidths;

	// Only attack if the weapon can reach the player
	// The weapon extends from the enemy's body edge by weaponHitBox width
	const float weaponReach = meleeAttack->get().weaponHitBox.w;
	return edgeGap <= weaponReach;
}

bool ProximityEntryTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return false;

	return glm::distance(enemy->center, player->center) <= radius;
}

bool ProximityExitTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return true; // No player = exited

	return glm::distance(enemy->center, player->center) > radius;
}

bool PoiseBreakTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	// Check for explicit "Hurting" state set by HealthSystem/CombatSystem
	if (auto stateComp = registry.tryGet<StateComponent>(entity)) {
		return stateComp->get().state == State::Hurting;
	}

	// Fallback to checking PoiseComponent directly (legacy or if HealthSystem doesn't set State)
	auto poise = registry.tryGet<PoiseComponent>(entity);
	return poise && poise->get().currentPoise <= 0.0f && poise->get().maxPoise > 0.0f;
}

bool RandomTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	return Utils::Random::getFloat(0.0f, 1.0f) <= chance;
}

bool ProximityAndLosEntryTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return false;

	if (glm::distance(enemy->center, player->center) > radius)
		return false;

	auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	auto projOpt = registry.tryGet<ProjectileComponent>(entity);

	return hasLosToAttacker(*enemy, *player, rangedOpt ? &rangedOpt->get() : nullptr,
	                        projOpt ? &projOpt->get() : nullptr);
}

bool LosLostTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return true; // No player = lost

	auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	auto projOpt = registry.tryGet<ProjectileComponent>(entity);

	if (hasLosToAttacker(*enemy, *player, rangedOpt ? &rangedOpt->get() : nullptr,
	                     projOpt ? &projOpt->get() : nullptr)) {
		losLostDuration = 0.0f;
		return false;
	}

	losLostDuration += deltaTime;
	return losLostDuration >= gracePeriod;
}

bool PlayerUnreachableTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return true; // No player = unreachable

	// Check if player is too far above the enemy (on a platform)
	// In screen coordinates, lower Y means higher position
	const float playerBottom = player->rect.y + player->rect.h;
	const float enemyBottom = enemy->rect.y + enemy->rect.h;

	// If player's feet are significantly above enemy's feet, player is unreachable
	return (enemyBottom - playerBottom) > maxHeightDifference;
}

bool PlayerUnreachableAndGroundedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return true; // No player = unreachable

	// 1. Check if unreachable (height difference)
	const float playerBottom = player->rect.y + player->rect.h;
	const float enemyBottom = enemy->rect.y + enemy->rect.h;
	bool unreachable = (enemyBottom - playerBottom) > maxHeightDifference;

	if (!unreachable)
		return false;

	// 2. Check if grounded (AND)
	auto playerView = registry.view<PlayerTag, GroundedComponent>();
	for (const auto playerEntity : playerView) {
		const auto &grounded = playerView.get<GroundedComponent>(playerEntity);
		return grounded.grounded;
	}

	return false;
}

bool MeleeProximityEntryTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return false;

	// Check distance
	if (glm::distance(enemy->center, player->center) > radius)
		return false;

	// Check if player is reachable (on roughly the same height)
	const float playerBottom = player->rect.y + player->rect.h;
	const float enemyBottom = enemy->rect.y + enemy->rect.h;

	// Don't chase if player is on a different platform (too high above OR too far below)
	if (std::abs(enemyBottom - playerBottom) > maxHeightDifference)
		return false;

	return true;
}

bool AnimationFinishedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto animComp = registry.tryGet<AnimationComponent>(entity);
	if (!animComp)
		return true; // No animation component, assume finished

	return animComp->get().hasFinished(animComp->get().getAnimation());
}

} // namespace GameEngine::ECS
