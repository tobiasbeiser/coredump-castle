#pragma once

#include "CoreActions.hpp"
#include "Engine/ECS/Components/StateMachine/StateMachineComponent.hpp"

namespace GameEngine::ECS {

// Actions

namespace Actions {

struct AimAtPlayerWithLOSAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct TriggerRangedAttackAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct CancelRangedAttackAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct RetreatFromPlayerAction final : StateAction {
	float retreatSpeed;

	explicit RetreatFromPlayerAction(float speed) : retreatSpeed(speed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct EvadeJumpAction final : StateAction {
	float jumpSpeed;

	explicit EvadeJumpAction(float speed) : jumpSpeed(speed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct RecordCornerAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

} // namespace Actions

namespace Transitions {

struct LineOfSightTransition : Transition {
	explicit LineOfSightTransition(AI::AIState target) { this->targetState = target; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct LosLostTransition final : Transition {
	mutable float losLostDuration = 0.0f;
	float gracePeriod;

	explicit LosLostTransition(AI::AIState target, float gracePeriodSeconds = 0.5f) : gracePeriod(gracePeriodSeconds)
	{
		this->targetState = target;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct VisibleTargetInRangeTransition final : AndTransition {
	VisibleTargetInRangeTransition(AI::AIState target, float radius) : AndTransition(target)
	{
		add<ProximityEntryTransition>(radius);
		add<LineOfSightTransition>();
	}
};

struct PlayerTooCloseTransition final : Transition {
	float closeRange;

	PlayerTooCloseTransition(AI::AIState target, float range) : closeRange(range)
	{
		this->targetState = target;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct SafeDistanceReachedTransition final : AndTransition {
	SafeDistanceReachedTransition(AI::AIState target, float safeDistance) : AndTransition(target)
	{
		add<ProximityExitTransition>(safeDistance);
		add<LineOfSightTransition>();
	}
};

struct IsGroundedTransition final : Transition {
	explicit IsGroundedTransition(AI::AIState target) { this->targetState = target; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct LandedTransition final : Transition {
	float minAirTime;

	explicit LandedTransition(const AI::AIState target, const float minTimeInAir = 0.2f) : minAirTime(minTimeInAir)
	{
		this->targetState = target;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct StuckTransition final : Transition {
	float threshold;
	explicit StuckTransition(const AI::AIState targetState, float velocityThreshold = 10.0f)
	    : threshold(velocityThreshold)
	{
		this->targetState = targetState;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

} // namespace Transitions

} // namespace GameEngine::ECS
