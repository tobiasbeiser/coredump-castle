#pragma once

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "Engine/Utils/Random.hpp"
#include "StateMachineComponent.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace GameEngine::ECS {

// ============================================================================
// TRANSITIONS
// ============================================================================

// Transition after a certain time duration
struct TimerTransition : Transition {
	float duration;

	TimerTransition(std::string targetState, float durationSeconds) : duration(durationSeconds)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override
	{
		auto sm = registry.tryGet<StateMachineComponent>(entity);
		if (!sm)
			return false;
		return sm->get().timeInState >= duration;
	}
};

// Transition based on player proximity (entry)
struct ProximityEntryTransition : Transition {
	float radius;

	ProximityEntryTransition(std::string targetState, float proximityRadius) : radius(proximityRadius)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

// Transition based on player proximity (exit)
struct ProximityExitTransition : Transition {
	float radius;

	ProximityExitTransition(std::string targetState, float proximityRadius) : radius(proximityRadius)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

// Transition based on health percentage
struct HealthThresholdTransition : Transition {
	float healthPercentage;
	bool lessThan; // true for <, false for >

	HealthThresholdTransition(std::string targetState, float healthPercent, bool isLessThan = true)
	    : healthPercentage(healthPercent), lessThan(isLessThan)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

// Transition based on poise break (stagger)
struct PoiseBreakTransition : Transition {
	PoiseBreakTransition(std::string targetState) { this->targetState = std::move(targetState); }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

// Transition based on player being stationary
struct PlayerStationaryTransition : Transition {
	float stationaryTime;

	PlayerStationaryTransition(std::string targetState, float timeStationary) : stationaryTime(timeStationary)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

// Random chance transition (useful for AI variety)
struct RandomTransition : Transition {
	float chance; // 0.0 to 1.0

	RandomTransition(std::string targetState, float transitionChance) : chance(transitionChance)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

// ============================================================================
// STATE ACTIONS
// ============================================================================

// Execute patrol behavior via PatrolSystem
struct PatrolAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Action: Set velocity
struct SetVelocityAction : StateAction {
	glm::vec2 velocity;

	explicit SetVelocityAction(glm::vec2 vel) : velocity(vel) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		if (auto vel = registry.tryGet<VelocityComponent>(entity)) {
			vel->get().velocity = velocity;
		}
	}
};

// Action: Trigger animation
struct TriggerAnimationAction : StateAction {
	std::string animationName;
	bool loop;

	TriggerAnimationAction(std::string animName, bool shouldLoop = false)
	    : animationName(std::move(animName)), loop(shouldLoop)
	{
	}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Forward declare the helper function
AnimationType stringToAnimationType(const std::string &animName);

// Action: Enable/disable a component
struct ToggleComponentAction : StateAction {
	enum class ComponentType { Chase, Patrol, Charge, Gravity, Collision };

	ComponentType component;
	bool enable;

	ToggleComponentAction(ComponentType comp, bool shouldEnable) : component(comp), enable(shouldEnable) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Action: Set movement speed
struct SetMovementSpeedAction : StateAction {
	float speed;

	explicit SetMovementSpeedAction(float newSpeed) : speed(newSpeed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		if (auto movement = registry.tryGet<MovementComponent>(entity)) {
			movement->get().maxSpeedX = speed;
		}
	}
};

// Action: Face player
struct FacePlayerAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Action: Move towards player
struct MoveTowardsPlayerAction : StateAction {
	float speed;

	explicit MoveTowardsPlayerAction(float moveSpeed) : speed(moveSpeed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Action: Execute attack
struct ExecuteAttackAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Action: Reset poise
struct ResetPoiseAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Action: Make invulnerable
struct SetInvulnerabilityAction : StateAction {
	bool invulnerable;

	explicit SetInvulnerabilityAction(bool isInvulnerable) : invulnerable(isInvulnerable) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// ============================================================================
// STATE DATA (for storing state-specific information)
// ============================================================================

// Data for telegraphing/windup states
struct TelegraphStateData : StateData {
	std::string nextAttackState;
	float windupDuration;

	TelegraphStateData(std::string nextState, float duration)
	    : nextAttackState(std::move(nextState)), windupDuration(duration)
	{
	}
};

// Data for attack states
struct AttackStateData : StateData {
	float damageMultiplier = 1.0f;
	float knockbackMultiplier = 1.0f;
	std::string recoveryState = "recover";
};

// Data for charge states
struct ChargeStateData : StateData {
	float chargeSpeed;
	float chargeDistance;
	glm::vec2 chargeDirection;

	ChargeStateData(float speed, float distance) : chargeSpeed(speed), chargeDistance(distance), chargeDirection(0.0f)
	{
	}
};

// ============================================================================
// RANGED ENEMY ACTIONS
// ============================================================================

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

// Trigger ranged attack if valid target exists
struct TriggerRangedAttackAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Reset ranged attack state (use as onExit when leaving combat)
struct CancelRangedAttackAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Aim at player with LOS check - sets targetSnapshot
struct AimAtPlayerWithLOSAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

// Composite action: stop, face, aim with LOS, and trigger attack
struct RangedAttackAction final : StateAction {
	StopAndFacePlayerAction stopAndFace;
	AimAtPlayerWithLOSAction aimWithLOS;
	TriggerRangedAttackAction triggerAttack;

	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		stopAndFace.execute(registry, entity, deltaTime);
		aimWithLOS.execute(registry, entity, deltaTime);
		triggerAttack.execute(registry, entity, deltaTime);
	}
};

// ============================================================================
// LOS-BASED TRANSITIONS (for ranged enemies)
// ============================================================================

// Transition requiring BOTH proximity AND line of sight
struct ProximityAndLosEntryTransition final : Transition {
	float radius;

	ProximityAndLosEntryTransition(std::string target, float proximityRadius) : radius(proximityRadius)
	{
		this->targetState = std::move(target);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

// Transition when LOS is lost for a duration
struct LosLostTransition final : Transition {
	mutable float losLostDuration = 0.0f;
	float gracePeriod;

	LosLostTransition(std::string target, float gracePeriodSeconds = 0.5f) : gracePeriod(gracePeriodSeconds)
	{
		this->targetState = std::move(target);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

} // namespace GameEngine::ECS
