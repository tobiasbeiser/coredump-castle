#pragma once

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "StateMachineComponent.hpp"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>

namespace GameEngine::ECS {

// ============================================================================
// TRANSITIONS
// ============================================================================

// Transition based on animation completion
struct AnimationTransition : Transition {
	std::string animationName;

	AnimationTransition(std::string targetState, std::string animName) : animationName(std::move(animName))
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override
	{
		// TODO: Check if animation is complete
		// This will need integration with your animation system
		return false;
	}
};

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

// Action: Set sprite/animation
struct SetSpriteAction : StateAction {
	int spriteIndex;

	explicit SetSpriteAction(int index) : spriteIndex(index) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		// TODO: Set sprite based on your rendering system
		// This might need to set animation state or sprite sheet index
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

} // namespace GameEngine::ECS
