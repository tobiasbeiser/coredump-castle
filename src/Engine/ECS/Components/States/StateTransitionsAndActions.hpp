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
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
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

struct ProximityEntryTransition : Transition {
	float radius;

	ProximityEntryTransition(std::string targetState, float proximityRadius) : radius(proximityRadius)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct ProximityExitTransition : Transition {
	float radius;

	ProximityExitTransition(std::string targetState, float proximityRadius) : radius(proximityRadius)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

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

struct PoiseBreakTransition : Transition {
	PoiseBreakTransition(std::string targetState) { this->targetState = std::move(targetState); }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct PlayerStationaryTransition : Transition {
	float stationaryTime;

	PlayerStationaryTransition(std::string targetState, float timeStationary) : stationaryTime(timeStationary)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct RandomTransition : Transition {
	float chance; // 0.0 to 1.0

	RandomTransition(std::string targetState, float transitionChance) : chance(transitionChance)
	{
		this->targetState = std::move(targetState);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct PatrolAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};
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

struct StopMovementAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		if (auto vel = registry.tryGet<VelocityComponent>(entity)) {
			vel->get().velocity.x = 0.0f; // Keep vertical for gravity
		}
		if (auto acc = registry.tryGet<AccelerationComponent>(entity)) {
			acc->get().acceleration.x = 0.0f;
		}
		if (auto input = registry.tryGet<InputComponent>(entity)) {
			input->get().horizontalDirection = 0.0f;
		}
	}
};

struct TriggerAnimationAction : StateAction {
	std::string animationName;
	bool loop;

	TriggerAnimationAction(std::string animName, bool shouldLoop = false)
	    : animationName(std::move(animName)), loop(shouldLoop)
	{
	}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

AnimationType stringToAnimationType(const std::string &animName);

struct ToggleComponentAction : StateAction {
	enum class ComponentType { Chase, Patrol, Charge, Gravity, Collision };

	ComponentType component;
	bool enable;

	ToggleComponentAction(ComponentType comp, const bool shouldEnable) : component(comp), enable(shouldEnable) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

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

struct FacePlayerAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};
struct MoveTowardsPlayerAction : StateAction {
	float speed;

	explicit MoveTowardsPlayerAction(float moveSpeed) : speed(moveSpeed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct ExecuteAttackAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};
struct ResetPoiseAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct SetInvulnerabilityAction : StateAction {
	bool invulnerable;

	explicit SetInvulnerabilityAction(bool isInvulnerable) : invulnerable(isInvulnerable) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct TelegraphStateData : StateData {
	std::string nextAttackState;
	float windupDuration;

	TelegraphStateData(std::string nextState, float duration)
	    : nextAttackState(std::move(nextState)), windupDuration(duration)
	{
	}
};

struct AttackStateData : StateData {
	float damageMultiplier = 1.0f;
	float knockbackMultiplier = 1.0f;
	std::string recoveryState = "recover";
};

struct ChargeStateData : StateData {
	float chargeSpeed;
	float chargeDistance;
	glm::vec2 chargeDirection;

	ChargeStateData(float speed, float distance) : chargeSpeed(speed), chargeDistance(distance), chargeDirection(0.0f)
	{
	}
};

struct StopAndFacePlayerAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};
struct RetreatFromPlayerAction final : StateAction {
	float speed;

	explicit RetreatFromPlayerAction(float retreatSpeed) : speed(retreatSpeed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct TriggerRangedAttackAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct CancelRangedAttackAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct CancelMeleeAttackAction : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct AimAtPlayerWithLOSAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct MeleeChaseAction final : StateAction {
	float speed;
	explicit MeleeChaseAction(float chaseSpeed) : speed(chaseSpeed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct MeleeAttackTransition final : Transition {
	MeleeAttackTransition(std::string target) { this->targetState = std::move(target); }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct ProximityAndLosEntryTransition final : Transition {
	float radius;

	ProximityAndLosEntryTransition(std::string target, float proximityRadius) : radius(proximityRadius)
	{
		this->targetState = std::move(target);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct LosLostTransition final : Transition {
	mutable float losLostDuration = 0.0f;
	float gracePeriod;

	LosLostTransition(std::string target, float gracePeriodSeconds = 0.5f) : gracePeriod(gracePeriodSeconds)
	{
		this->targetState = std::move(target);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct PlayerNotGroundedTransition final : Transition {
	PlayerNotGroundedTransition(std::string target) { this->targetState = std::move(target); }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct PlayerUnreachableTransition final : Transition {
	float maxHeightDifference;

	PlayerUnreachableTransition(std::string target, float maxHeight = 32.0f) : maxHeightDifference(maxHeight)
	{
		this->targetState = std::move(target);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct MeleeProximityEntryTransition final : Transition {
	float radius;
	float maxHeightDifference;

	MeleeProximityEntryTransition(std::string target, float proximityRadius, float maxHeight = 32.0f)
	    : radius(proximityRadius), maxHeightDifference(maxHeight)
	{
		this->targetState = std::move(target);
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

} // namespace GameEngine::ECS
