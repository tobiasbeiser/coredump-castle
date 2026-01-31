#pragma once

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/States/StateTransitionsAndActions.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "Engine/Utils/Random.hpp"
#include "Game/Factories/ProjectileFactory.hpp"

#include <SDL3/SDL_log.h>

namespace GameEngine::ECS {

// Execute normal patrol behavior via PatrolSystem
struct PatrolAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		Systems::PatrolSystem::updateEntity(registry, entity); 
	}
};

// Move away from player at specified speed
struct RetreatFromPlayerAction final : StateAction {
	float speed;

	explicit RetreatFromPlayerAction(float retreatSpeed) : speed(retreatSpeed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		auto enemyPos = registry.tryGet<PositionComponent>(entity);
		auto enemyInput = registry.tryGet<InputComponent>(entity);
		auto enemyMovement = registry.tryGet<MovementComponent>(entity);
		auto enemyRender = registry.tryGet<RenderComponent>(entity);

		if (!enemyPos || !enemyInput || !enemyMovement)
			return;

		auto playerView = registry.view<PlayerTag, PositionComponent>();
		for (const auto playerEntity : playerView) {
			const auto &playerPos = playerView.get<PositionComponent>(playerEntity);

			// Move AWAY from player
			const float direction = playerPos.position.x > enemyPos->get().position.x ? -1.0f : 1.0f;
			enemyInput->get().horizontalDirection = direction;
			enemyMovement->get().maxSpeedX = speed;

			// Face player while retreating
			if (enemyRender) {
				enemyRender->get().setFacingLeft(playerPos.position.x < enemyPos->get().position.x);
			}
			break;
		}
	}
};

// Stop moving and face the player
struct StopAndFacePlayerAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		auto posOpt = registry.tryGet<PositionComponent>(entity);
		auto inputOpt = registry.tryGet<InputComponent>(entity);
		auto renderOpt = registry.tryGet<RenderComponent>(entity);
		auto hurtBoxOpt = registry.tryGet<HurtBoxComponent>(entity);
		auto transformOpt = registry.tryGet<TransformComponent>(entity);

		if (!posOpt || !inputOpt || !renderOpt || !hurtBoxOpt)
			return;

		const glm::vec2 scale = transformOpt ? transformOpt->get().scale : glm::vec2{1.0f, 1.0f};
		const SDL_FRect enemyRect =
		    Utils::HitBoxUtils::getHitBox(posOpt->get().position, hurtBoxOpt->get().hurtBox, &scale);
		const glm::vec2 enemyCenter = {enemyRect.x + enemyRect.w * 0.5f, enemyRect.y + enemyRect.h * 0.5f};

		auto playerView = registry.view<PlayerTag, PositionComponent, HurtBoxComponent>();
		for (const auto playerEntity : playerView) {
			const auto &playerPos = playerView.get<PositionComponent>(playerEntity);
			const auto &playerHurtBox = playerView.get<HurtBoxComponent>(playerEntity);
			auto playerTransformOpt = registry.tryGet<TransformComponent>(playerEntity);
			const glm::vec2 playerScale = playerTransformOpt ? playerTransformOpt->get().scale : glm::vec2{1.0f, 1.0f};

			const SDL_FRect playerRect =
			    Utils::HitBoxUtils::getHitBox(playerPos.position, playerHurtBox.hurtBox, &playerScale);
			const glm::vec2 playerCenter = {playerRect.x + playerRect.w * 0.5f, playerRect.y + playerRect.h * 0.5f};

			renderOpt->get().setFacingLeft(playerCenter.x < enemyCenter.x);
			inputOpt->get().horizontalDirection = 0.0f;
			break;
		}
	}
};

// Set target snapshot to player position (no LOS check)
struct UpdateTargetSnapshotAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
		if (!rangedOpt)
			return;

		auto playerView = registry.view<PlayerTag, PositionComponent, HurtBoxComponent>();
		for (const auto playerEntity : playerView) {
			const auto &playerPos = playerView.get<PositionComponent>(playerEntity);
			const auto &playerHurtBox = playerView.get<HurtBoxComponent>(playerEntity);
			auto playerTransformOpt = registry.tryGet<TransformComponent>(playerEntity);
			const glm::vec2 playerScale = playerTransformOpt ? playerTransformOpt->get().scale : glm::vec2{1.0f, 1.0f};

			const SDL_FRect playerRect =
			    Utils::HitBoxUtils::getHitBox(playerPos.position, playerHurtBox.hurtBox, &playerScale);
			rangedOpt->get().targetSnapshot = {playerRect.x + playerRect.w * 0.5f, playerRect.y + playerRect.h * 0.5f};
			break;
		}
	}
};

// Set target snapshot only if clear line of sight exists
struct UpdateTargetSnapshotWithLOSAction final : StateAction {
	void execute(Registry &registry, const Entity entity, const float deltaTime) override
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
		const SDL_FRect enemyRect =
		    Utils::HitBoxUtils::getHitBox(posOpt->get().position, hurtBoxOpt->get().hurtBox, &scale);

		glm::vec2 projectileSize = {0.0f, 0.0f};
		glm::vec2 hitboxOffset = {0.0f, 0.0f};
		if (projectileOpt) {
			const auto &tilesetPath = projectileOpt->get().projectileTilesetPath;
			if (!tilesetPath.empty()) {
				const auto &projectileData = ProjectileFactory::getCachedProjectileData(tilesetPath);
				projectileSize = {projectileData.hitbox.w, projectileData.hitbox.h};
				hitboxOffset = {projectileData.hitbox.x, projectileData.hitbox.y};
			}
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

			// Use actual facing direction from RenderComponent (same as ProjectileSystem)
			const bool facingLeft = renderOpt->get().isFacingLeft();
			glm::vec2 spawnPosition = posOpt->get().position;
			if (facingLeft) {
				spawnPosition.x += (hurtBoxOpt->get().hurtBox.x * scale.x) - rangedOpt->get().projectileOffset.x;
			} else {
				spawnPosition.x += (hurtBoxOpt->get().hurtBox.x * scale.x) + rangedOpt->get().projectileOffset.x;
			}
			spawnPosition.y += rangedOpt->get().projectileOffset.y;

			const glm::vec2 rayStart = spawnPosition + hitboxOffset + (projectileSize * 0.5f);

			// Get the actual target point that has clear LOS (center or head)
			auto targetOpt = Utils::DetectionRangeUtils::getTargetWithLineOfSight(
			    rayStart, playerCenter, playerHurtBox.hurtBox, playerScale, projectileSize);
			if (targetOpt.has_value()) {
				rangedOpt->get().targetSnapshot = targetOpt.value();
				return;
			}
			rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
			break;
		}
	}
};

// Trigger ranged attack if valid target exists and not already attacking
struct TriggerRangedAttackAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override
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

		// No valid target - cancel any in-progress attack and don't start new one
		if (ranged.targetSnapshot == glm::vec2(0.0f)) {
			input.attackPressed = false;
			attack.attackTimer.reset(); // Cancel in-progress attack too
			return;
		}

		if (!Systems::AttackSystem::isAttacking(attack)) {
			input.attackPressed = true;
			attack.activeWindUpLimit = Utils::Random::getFloat(attack.windUpLimit * 0.5f, attack.windUpLimit);
		}
	}
};

// Reset ranged attack state (use as onExit when leaving combat)
struct ResetRangedAttackAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		auto inputOpt = registry.tryGet<InputComponent>(entity);
		auto attackOpt = registry.tryGet<AttackComponent>(entity);
		auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);

		if (inputOpt) {
			inputOpt->get().attackPressed = false;
		}
		if (attackOpt) {
			attackOpt->get().attackTimer.reset();
		}
		if (rangedOpt) {
			rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
			rangedOpt->get().projectileSpawned = false;
		}
	}
};

// Convenience action combining stop, face, aim with LOS, and trigger attack
struct RangedAttackAction final : StateAction {
	StopAndFacePlayerAction stopAndFace;
	UpdateTargetSnapshotWithLOSAction updateTargetWithLOS;
	TriggerRangedAttackAction triggerAttack;

	void execute(Registry &registry, const Entity entity, const float deltaTime) override
	{
		stopAndFace.execute(registry, entity, deltaTime);
		updateTargetWithLOS.execute(registry, entity, deltaTime);
		triggerAttack.execute(registry, entity, deltaTime);
	}
};

// Transition that requires BOTH proximity AND line of sight to enter combat
// This prevents flickering when player is in range but behind cover
struct ProximityAndLosEntryTransition final : Transition {
	float radius;

	ProximityAndLosEntryTransition(std::string target, float proximityRadius) : radius(proximityRadius)
	{
		this->targetState = std::move(target);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override
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

		// Get projectile size for LOS check
		glm::vec2 projectileSize = {0.0f, 0.0f};
		glm::vec2 hitboxOffset = {0.0f, 0.0f};
		if (projectileOpt) {
			const auto &tilesetPath = projectileOpt->get().projectileTilesetPath;
			if (!tilesetPath.empty()) {
				const auto &projectileData = ProjectileFactory::getCachedProjectileData(tilesetPath);
				projectileSize = {projectileData.hitbox.w, projectileData.hitbox.h};
				hitboxOffset = {projectileData.hitbox.x, projectileData.hitbox.y};
			}
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

			// First check: is player in range?
			const float distance = glm::distance(enemyCenter, playerCenter);
			if (distance > radius) {
				return false;
			}

			// Second check: do we have LOS?
			glm::vec2 spawnPosition = posOpt->get().position;
			const bool facingLeft = (playerCenter.x - posOpt->get().position.x) < 0.0f;
			float xOffset = (hurtBoxOpt->get().hurtBox.x * scale.x);
			if (rangedOpt) {
				xOffset += facingLeft ? -rangedOpt->get().projectileOffset.x : rangedOpt->get().projectileOffset.x;
				spawnPosition.y += rangedOpt->get().projectileOffset.y;
			}
			spawnPosition.x += xOffset;

			const glm::vec2 rayStart = spawnPosition + hitboxOffset + (projectileSize * 0.5f);

			bool hasLOS = Utils::DetectionRangeUtils::checkLineOfSight(
			    rayStart, playerCenter, playerHurtBox.hurtBox, playerScale, projectileSize);

			return hasLOS;  // Only transition if we have both proximity AND LOS
		}

		return false;
	}
};

// Transition that fires when line of sight to player is lost for a duration
struct LosLostTransition final : Transition {
	mutable float losLostDuration = 0.0f;
	float gracePeriod;  // Time without LOS before transitioning

	LosLostTransition(std::string target, float gracePeriodSeconds = 0.5f) : gracePeriod(gracePeriodSeconds)
	{
		this->targetState = std::move(target);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override
	{
		auto posOpt = registry.tryGet<PositionComponent>(entity);
		auto hurtBoxOpt = registry.tryGet<HurtBoxComponent>(entity);
		auto transformOpt = registry.tryGet<TransformComponent>(entity);
		auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
		auto projectileOpt = registry.tryGet<ProjectileComponent>(entity);

		if (!posOpt || !hurtBoxOpt)
			return false;

		const glm::vec2 scale = transformOpt ? transformOpt->get().scale : glm::vec2{1.0f, 1.0f};

		// Get projectile size for LOS check
		glm::vec2 projectileSize = {0.0f, 0.0f};
		glm::vec2 hitboxOffset = {0.0f, 0.0f};
		if (projectileOpt) {
			const auto &tilesetPath = projectileOpt->get().projectileTilesetPath;
			if (!tilesetPath.empty()) {
				const auto &projectileData = ProjectileFactory::getCachedProjectileData(tilesetPath);
				projectileSize = {projectileData.hitbox.w, projectileData.hitbox.h};
				hitboxOffset = {projectileData.hitbox.x, projectileData.hitbox.y};
			}
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

			// Calculate ray start position (from muzzle)
			glm::vec2 spawnPosition = posOpt->get().position;
			const bool facingLeft = (playerCenter.x - posOpt->get().position.x) < 0.0f;
			float xOffset = (hurtBoxOpt->get().hurtBox.x * scale.x);
			if (rangedOpt) {
				xOffset += facingLeft ? -rangedOpt->get().projectileOffset.x : rangedOpt->get().projectileOffset.x;
				spawnPosition.y += rangedOpt->get().projectileOffset.y;
			}
			spawnPosition.x += xOffset;

			const glm::vec2 rayStart = spawnPosition + hitboxOffset + (projectileSize * 0.5f);

			bool hasLOS = Utils::DetectionRangeUtils::checkLineOfSight(
			    rayStart, playerCenter, playerHurtBox.hurtBox, playerScale, projectileSize);

			if (hasLOS) {
				// Reset timer when we have LOS
				losLostDuration = 0.0f;
				return false;
			}

			// Accumulate time without LOS
			losLostDuration += deltaTime;
			if (losLostDuration >= gracePeriod) {
				losLostDuration = 0.0f;
				return true;  // Transition to patrol
			}
			return false;
		}

		// No player found - transition
		return true;
	}
};

} // namespace GameEngine::ECS
