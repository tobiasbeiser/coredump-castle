#include "StateTransitionsAndActions.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PoiseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
#include "Engine/Utils/Random.hpp"
#include "Game/Factories/ProjectileFactory.hpp"
#include <SDL3/SDL_log.h>
#include <cmath>

namespace GameEngine::ECS {

// ============================================================================
// STATE ACTION IMPLEMENTATIONS
// ============================================================================

void PatrolAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	Systems::PatrolSystem::updateEntity(registry, entity);
}

// ============================================================================
// TRANSITION IMPLEMENTATIONS
// ============================================================================

bool ProximityEntryTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	// Get enemy position
	auto enemyPos = registry.tryGet<PositionComponent>(entity);
	auto enemyHurtBox = registry.tryGet<HurtBoxComponent>(entity);
	auto enemyTransform = registry.tryGet<TransformComponent>(entity);

	if (!enemyPos || !enemyHurtBox)
		return false;

	glm::vec2 enemyScale = enemyTransform ? enemyTransform->get().scale : glm::vec2{1.0f, 1.0f};
	SDL_FRect enemyRect =
	    Utils::HitBoxUtils::getHitBox(enemyPos->get().position, enemyHurtBox->get().hurtBox, &enemyScale);
	glm::vec2 enemyCenter = {enemyRect.x + enemyRect.w * 0.5f, enemyRect.y + enemyRect.h * 0.5f};

	// Find player
	auto playerView = registry.view<PlayerTag, PositionComponent, HurtBoxComponent>();
	for (const auto playerEntity : playerView) {
		auto &playerPos = playerView.get<PositionComponent>(playerEntity);
		auto &playerHurtBox = playerView.get<HurtBoxComponent>(playerEntity);

		auto playerTransform = registry.tryGet<TransformComponent>(playerEntity);
		glm::vec2 playerScale = playerTransform ? playerTransform->get().scale : glm::vec2{1.0f, 1.0f};

		SDL_FRect playerRect = Utils::HitBoxUtils::getHitBox(playerPos.position, playerHurtBox.hurtBox, &playerScale);
		glm::vec2 playerCenter = {playerRect.x + playerRect.w * 0.5f, playerRect.y + playerRect.h * 0.5f};

		float distance = glm::distance(enemyCenter, playerCenter);
		return distance <= radius;
	}

	return false;
}

bool ProximityExitTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	// Get enemy position
	auto enemyPos = registry.tryGet<PositionComponent>(entity);
	auto enemyHurtBox = registry.tryGet<HurtBoxComponent>(entity);
	auto enemyTransform = registry.tryGet<TransformComponent>(entity);

	if (!enemyPos || !enemyHurtBox)
		return false;

	glm::vec2 enemyScale = enemyTransform ? enemyTransform->get().scale : glm::vec2{1.0f, 1.0f};
	SDL_FRect enemyRect =
	    Utils::HitBoxUtils::getHitBox(enemyPos->get().position, enemyHurtBox->get().hurtBox, &enemyScale);
	glm::vec2 enemyCenter = {enemyRect.x + enemyRect.w * 0.5f, enemyRect.y + enemyRect.h * 0.5f};

	// Find player
	auto playerView = registry.view<PlayerTag, PositionComponent, HurtBoxComponent>();
	for (const auto playerEntity : playerView) {
		auto &playerPos = playerView.get<PositionComponent>(playerEntity);
		auto &playerHurtBox = playerView.get<HurtBoxComponent>(playerEntity);

		auto playerTransform = registry.tryGet<TransformComponent>(playerEntity);
		glm::vec2 playerScale = playerTransform ? playerTransform->get().scale : glm::vec2{1.0f, 1.0f};

		SDL_FRect playerRect = Utils::HitBoxUtils::getHitBox(playerPos.position, playerHurtBox.hurtBox, &playerScale);
		glm::vec2 playerCenter = {playerRect.x + playerRect.w * 0.5f, playerRect.y + playerRect.h * 0.5f};

		float distance = glm::distance(enemyCenter, playerCenter);
		return distance > radius;
	}

	return true; // No player found, consider it "exited"
}

bool HealthThresholdTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto health = registry.tryGet<HealthComponent>(entity);
	if (!health)
		return false;

	float currentPercent = health->get().health / static_cast<float>(health->get().maxHealth);

	if (lessThan) {
		return currentPercent < healthPercentage;
	} else {
		return currentPercent > healthPercentage;
	}
}

bool PoiseBreakTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto poise = registry.tryGet<PoiseComponent>(entity);
	if (!poise)
		return false;

	// Check if poise was just broken
	return poise->get().currentPoise <= 0.0f && poise->get().maxPoise > 0.0f;
}

bool PlayerStationaryTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	// Find player
	auto playerView = registry.view<PlayerTag, VelocityComponent>();
	for (const auto playerEntity : playerView) {
		auto &playerVel = playerView.get<VelocityComponent>(playerEntity);

		// Check if player is stationary
		float velocityMagnitude = glm::length(playerVel.velocity);
		if (velocityMagnitude < 0.1f) {
			// TODO: Track time stationary - might need a component for this
			return true;
		}
	}

	return false;
}

bool RandomTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	return Utils::Random::getFloat(0.0f, 1.0f) <= chance;
}

// ============================================================================
// STATE ACTION IMPLEMENTATIONS
// ============================================================================

void ToggleComponentAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	switch (component) {
	case ComponentType::Chase:
		if (enable && !registry.has<ChaseComponent>(entity)) {
			// Would need to know default values to add it
		} else if (!enable && registry.has<ChaseComponent>(entity)) {
			registry.remove<ChaseComponent>(entity);
		}
		break;

	case ComponentType::Patrol:
		if (enable && !registry.has<PatrolComponent>(entity)) {
			// Would need to know default values to add it
		} else if (!enable && registry.has<PatrolComponent>(entity)) {
			registry.remove<PatrolComponent>(entity);
		}
		break;

	case ComponentType::Charge:
		if (enable && !registry.has<ChargeComponent>(entity)) {
			registry.registerComponent<ChargeComponent>(entity, ChargeComponent{});
		} else if (!enable && registry.has<ChargeComponent>(entity)) {
			registry.remove<ChargeComponent>(entity);
		}
		break;

	case ComponentType::Gravity:
		if (enable && !registry.has<GravityComponent>(entity)) {
			registry.registerComponent<GravityComponent>(entity, GravityComponent{});
		} else if (!enable && registry.has<GravityComponent>(entity)) {
			registry.remove<GravityComponent>(entity);
		}
		break;

	case ComponentType::Collision:
		// CollisionComponent doesn't have an 'enabled' field
		// If you need to disable collision, remove CollisionComponent from entity
		if (enable && !registry.has<CollisionComponent>(entity)) {
			// Add collision if enabling (need proper collision box data)
		} else if (!enable && registry.has<CollisionComponent>(entity)) {
			registry.remove<CollisionComponent>(entity);
		}
		break;
	}
}

void FacePlayerAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto enemyPos = registry.tryGet<PositionComponent>(entity);
	auto enemyRender = registry.tryGet<RenderComponent>(entity);

	if (!enemyPos || !enemyRender)
		return;

	// Find player
	auto playerView = registry.view<PlayerTag, PositionComponent>();
	for (const auto playerEntity : playerView) {
		auto &playerPos = playerView.get<PositionComponent>(playerEntity);

		bool facingLeft = playerPos.position.x < enemyPos->get().position.x;
		enemyRender->get().setFacingLeft(facingLeft);
		break; // Only one player
	}
}

void MoveTowardsPlayerAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto enemyPos = registry.tryGet<PositionComponent>(entity);
	auto enemyInput = registry.tryGet<InputComponent>(entity);
	auto enemyRender = registry.tryGet<RenderComponent>(entity);

	if (!enemyPos || !enemyInput)
		return;

	// Find player
	auto playerView = registry.view<PlayerTag, PositionComponent>();
	for (const auto playerEntity : playerView) {
		auto &playerPos = playerView.get<PositionComponent>(playerEntity);

		float direction = playerPos.position.x > enemyPos->get().position.x ? 1.0f : -1.0f;
		enemyInput->get().horizontalDirection = direction;

		if (enemyRender) {
			enemyRender->get().setFacingLeft(direction < 0);
		}
		break;
	}
}

void ExecuteAttackAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (auto input = registry.tryGet<InputComponent>(entity)) {
		input->get().attackPressed = true;
		SDL_Log("[ExecuteAttackAction] Entity %u attackPressed set to true", static_cast<unsigned int>(entity));
	} else {
		SDL_Log("[ExecuteAttackAction] Entity %u has no InputComponent!", static_cast<unsigned int>(entity));
	}
}

void ResetPoiseAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (auto poise = registry.tryGet<PoiseComponent>(entity)) {
		poise->get().currentPoise = poise->get().maxPoise;
	}
}

void SetInvulnerabilityAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	// TODO: Implement invulnerability component/flag
	// This might be a flag in HealthComponent or a separate InvulnerabilityComponent
}

// ============================================================================
// Animation Trigger
// ============================================================================

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
	if (animName == "dead" || animName == "death" || animName == "dying")
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
	if (!animComp) {
		SDL_Log("[TriggerAnimation] Entity %u has no AnimationComponent!", static_cast<unsigned int>(entity));
		return;
	}

	const AnimationType animType = stringToAnimationType(animationName);
	if (animType != AnimationType::None) {
		SDL_Log("[TriggerAnimation] Entity %u setting animation '%s' (type %d)", static_cast<unsigned int>(entity),
		        animationName.c_str(), static_cast<int>(animType));
		animComp->get().setAnimation(animType);
	} else {
		SDL_Log("[TriggerAnimation] Entity %u - unknown animation name '%s'", static_cast<unsigned int>(entity),
		        animationName.c_str());
	}
}

// ============================================================================
// RANGED ENEMY ACTION IMPLEMENTATIONS
// ============================================================================

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

	// Check if player is dead
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

	// No valid target - cancel attack
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
	if (auto inputOpt = registry.tryGet<InputComponent>(entity)) {
		inputOpt->get().attackPressed = false;
	}
	if (auto attackOpt = registry.tryGet<AttackComponent>(entity)) {
		attackOpt->get().attackTimer.reset();
	}
	if (auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity)) {
		rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
		rangedOpt->get().projectileSpawned = false;
	}
}

void AimAtPlayerWithLOSAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto posOpt = registry.tryGet<PositionComponent>(entity);
	auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	auto hurtBoxOpt = registry.tryGet<HurtBoxComponent>(entity);
	auto transformOpt = registry.tryGet<TransformComponent>(entity);
	auto projectileOpt = registry.tryGet<ProjectileComponent>(entity);
	auto renderOpt = registry.tryGet<RenderComponent>(entity);

	if (!posOpt || !rangedOpt || !hurtBoxOpt || !renderOpt)
		return;

	const glm::vec2 scale = transformOpt ? transformOpt->get().scale : glm::vec2{1.0f, 1.0f};

	glm::vec2 projectileSize = {0.0f, 0.0f};
	glm::vec2 hitboxOffset = {0.0f, 0.0f};
	if (projectileOpt && !projectileOpt->get().projectileTilesetPath.empty()) {
		const auto &projectileData =
		    ProjectileFactory::getCachedProjectileData(projectileOpt->get().projectileTilesetPath);
		projectileSize = {projectileData.hitbox.w, projectileData.hitbox.h};
		hitboxOffset = {projectileData.hitbox.x, projectileData.hitbox.y};
	}

	auto playerView = registry.view<PlayerTag, PositionComponent, HurtBoxComponent>();
	for (const auto playerEntity : playerView) {
		const auto &playerPos = playerView.get<PositionComponent>(playerEntity);
		const auto &playerHurtBox = playerView.get<HurtBoxComponent>(playerEntity);
		auto playerTransformOpt = registry.tryGet<TransformComponent>(playerEntity);
		const glm::vec2 playerScale = playerTransformOpt ? playerTransformOpt->get().scale : glm::vec2{1.0f, 1.0f};

		const SDL_FRect playerRect =
		    Utils::HitBoxUtils::getHitBox(playerPos.position, playerHurtBox.hurtBox, &playerScale);
		const glm::vec2 playerCenter = {playerRect.x + playerRect.w * 0.5f, playerRect.y + playerRect.h * 0.5f};

		// Use actual facing direction (same as ProjectileSystem)
		const bool facingLeft = renderOpt->get().isFacingLeft();
		glm::vec2 spawnPosition = posOpt->get().position;
		if (facingLeft) {
			spawnPosition.x += (hurtBoxOpt->get().hurtBox.x * scale.x) - rangedOpt->get().projectileOffset.x;
		} else {
			spawnPosition.x += (hurtBoxOpt->get().hurtBox.x * scale.x) + rangedOpt->get().projectileOffset.x;
		}
		spawnPosition.y += rangedOpt->get().projectileOffset.y;

		const glm::vec2 rayStart = spawnPosition + hitboxOffset + (projectileSize * 0.5f);

		auto targetOpt = Utils::DetectionRangeUtils::getTargetWithLineOfSight(rayStart, playerCenter,
		                                                                       playerHurtBox.hurtBox, playerScale, projectileSize);
		if (targetOpt.has_value()) {
			rangedOpt->get().targetSnapshot = targetOpt.value();
			return;
		}
		rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
		break;
	}
}

// ============================================================================
// LOS TRANSITION IMPLEMENTATIONS
// ============================================================================

bool ProximityAndLosEntryTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto posOpt = registry.tryGet<PositionComponent>(entity);
	auto hurtBoxOpt = registry.tryGet<HurtBoxComponent>(entity);
	auto transformOpt = registry.tryGet<TransformComponent>(entity);
	auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	auto projectileOpt = registry.tryGet<ProjectileComponent>(entity);

	if (!posOpt || !hurtBoxOpt)
		return false;

	const glm::vec2 scale = transformOpt ? transformOpt->get().scale : glm::vec2{1.0f, 1.0f};
	const SDL_FRect enemyRect =
	    Utils::HitBoxUtils::getHitBox(posOpt->get().position, hurtBoxOpt->get().hurtBox, &scale);
	const glm::vec2 enemyCenter = {enemyRect.x + enemyRect.w * 0.5f, enemyRect.y + enemyRect.h * 0.5f};

	glm::vec2 projectileSize = {0.0f, 0.0f};
	glm::vec2 hitboxOffset = {0.0f, 0.0f};
	if (projectileOpt && !projectileOpt->get().projectileTilesetPath.empty()) {
		const auto &projectileData =
		    ProjectileFactory::getCachedProjectileData(projectileOpt->get().projectileTilesetPath);
		projectileSize = {projectileData.hitbox.w, projectileData.hitbox.h};
		hitboxOffset = {projectileData.hitbox.x, projectileData.hitbox.y};
	}

	auto playerView = registry.view<PlayerTag, PositionComponent, HurtBoxComponent>();
	for (const auto playerEntity : playerView) {
		const auto &playerPos = playerView.get<PositionComponent>(playerEntity);
		const auto &playerHurtBox = playerView.get<HurtBoxComponent>(playerEntity);
		auto playerTransformOpt = registry.tryGet<TransformComponent>(playerEntity);
		const glm::vec2 playerScale = playerTransformOpt ? playerTransformOpt->get().scale : glm::vec2{1.0f, 1.0f};

		const SDL_FRect playerRect =
		    Utils::HitBoxUtils::getHitBox(playerPos.position, playerHurtBox.hurtBox, &playerScale);
		const glm::vec2 playerCenter = {playerRect.x + playerRect.w * 0.5f, playerRect.y + playerRect.h * 0.5f};

		// Check proximity first
		if (glm::distance(enemyCenter, playerCenter) > radius) {
			return false;
		}

		// Check LOS
		glm::vec2 spawnPosition = posOpt->get().position;
		const bool facingLeft = (playerCenter.x - posOpt->get().position.x) < 0.0f;
		float xOffset = (hurtBoxOpt->get().hurtBox.x * scale.x);
		if (rangedOpt) {
			xOffset += facingLeft ? -rangedOpt->get().projectileOffset.x : rangedOpt->get().projectileOffset.x;
			spawnPosition.y += rangedOpt->get().projectileOffset.y;
		}
		spawnPosition.x += xOffset;

		const glm::vec2 rayStart = spawnPosition + hitboxOffset + (projectileSize * 0.5f);
		return Utils::DetectionRangeUtils::checkLineOfSight(rayStart, playerCenter, playerHurtBox.hurtBox,
		                                                    playerScale, projectileSize);
	}

	return false;
}

bool LosLostTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto posOpt = registry.tryGet<PositionComponent>(entity);
	auto hurtBoxOpt = registry.tryGet<HurtBoxComponent>(entity);
	auto transformOpt = registry.tryGet<TransformComponent>(entity);
	auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	auto projectileOpt = registry.tryGet<ProjectileComponent>(entity);

	if (!posOpt || !hurtBoxOpt)
		return false;

	const glm::vec2 scale = transformOpt ? transformOpt->get().scale : glm::vec2{1.0f, 1.0f};

	glm::vec2 projectileSize = {0.0f, 0.0f};
	glm::vec2 hitboxOffset = {0.0f, 0.0f};
	if (projectileOpt && !projectileOpt->get().projectileTilesetPath.empty()) {
		const auto &projectileData =
		    ProjectileFactory::getCachedProjectileData(projectileOpt->get().projectileTilesetPath);
		projectileSize = {projectileData.hitbox.w, projectileData.hitbox.h};
		hitboxOffset = {projectileData.hitbox.x, projectileData.hitbox.y};
	}

	auto playerView = registry.view<PlayerTag, PositionComponent, HurtBoxComponent>();
	for (const auto playerEntity : playerView) {
		const auto &playerPos = playerView.get<PositionComponent>(playerEntity);
		const auto &playerHurtBox = playerView.get<HurtBoxComponent>(playerEntity);
		auto playerTransformOpt = registry.tryGet<TransformComponent>(playerEntity);
		const glm::vec2 playerScale = playerTransformOpt ? playerTransformOpt->get().scale : glm::vec2{1.0f, 1.0f};

		const SDL_FRect playerRect =
		    Utils::HitBoxUtils::getHitBox(playerPos.position, playerHurtBox.hurtBox, &playerScale);
		const glm::vec2 playerCenter = {playerRect.x + playerRect.w * 0.5f, playerRect.y + playerRect.h * 0.5f};

		glm::vec2 spawnPosition = posOpt->get().position;
		const bool facingLeft = (playerCenter.x - posOpt->get().position.x) < 0.0f;
		float xOffset = (hurtBoxOpt->get().hurtBox.x * scale.x);
		if (rangedOpt) {
			xOffset += facingLeft ? -rangedOpt->get().projectileOffset.x : rangedOpt->get().projectileOffset.x;
			spawnPosition.y += rangedOpt->get().projectileOffset.y;
		}
		spawnPosition.x += xOffset;

		const glm::vec2 rayStart = spawnPosition + hitboxOffset + (projectileSize * 0.5f);

		bool hasLOS = Utils::DetectionRangeUtils::checkLineOfSight(rayStart, playerCenter, playerHurtBox.hurtBox,
		                                                           playerScale, projectileSize);

		if (hasLOS) {
			losLostDuration = 0.0f;
			return false;
		}

		losLostDuration += deltaTime;
		if (losLostDuration >= gracePeriod) {
			losLostDuration = 0.0f;
			return true;
		}
		return false;
	}

	return true;  // No player found - transition
}

} // namespace GameEngine::ECS
