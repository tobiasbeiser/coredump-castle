#pragma once

#include "CoreActions.hpp"
#include "Engine/ECS/Components/StateMachine/StateMachineComponent.hpp"

struct RangedAttackComponent;
struct ProjectileComponent;

namespace GameEngine::ECS {
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

struct EscapeThroughPlayerAction final : StateAction {
	float escapeSpeed;
	float storedDirection = 0.0f;
	bool hasStarted = false;

	explicit EscapeThroughPlayerAction(float speed) : escapeSpeed(speed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct ApplyProximitySlowAction final : StateAction {
	float slowRange;
	float slowMultiplier;
	float slowDuration;

	explicit ApplyProximitySlowAction(float range, float multiplier, float duration)
	    : slowRange(range), slowMultiplier(multiplier), slowDuration(duration)
	{
	}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct ApplyConcussionVignetteAction final : StateAction {
	float effectRange;
	float effectDuration;

	explicit ApplyConcussionVignetteAction(float range, float duration) : effectRange(range), effectDuration(duration)
	{
	}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct ApplyCameraShakeAction final : StateAction {
	float amplitude;
	float duration;

	explicit ApplyCameraShakeAction(float shakeAmplitude, float shakeDuration)
	    : amplitude(shakeAmplitude), duration(shakeDuration)
	{
	}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct PhaseShiftAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct RestoreOpacityAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct RecordCornerAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct StartSpreadTelegraphAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct StopSpreadTelegraphAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

} // namespace Actions

namespace Transitions {

struct LineOfSightTransition final : Transition {
	explicit LineOfSightTransition(const AI::AIState target) { this->targetState = target; }

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

	PlayerTooCloseTransition(const AI::AIState target, const float range) : closeRange(range)
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
	float requiredTime;
	mutable float currentStuckTime = 0.0f;

	explicit StuckTransition(const AI::AIState targetState, float velocityThreshold = 10.0f, float stuckTimeout = 0.2f)
	    : threshold(velocityThreshold), requiredTime(stuckTimeout)
	{
		this->targetState = targetState;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct CanRetreatTransition final : Transition {
	explicit CanRetreatTransition(AI::AIState targetState) { this->targetState = targetState; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct CorneredRetreatTransition final : AndTransition {
	CorneredRetreatTransition(AI::AIState target, float retreatTriggerRange) : AndTransition(target)
	{
		add<PlayerTooCloseTransition>(retreatTriggerRange);
		add<CanRetreatTransition>();
	}
};

struct SafePassedPlayerTransition final : Transition {
	float safetyDistance;

	explicit SafePassedPlayerTransition(AI::AIState target, float distance = 100.0f) : safetyDistance(distance)
	{
		this->targetState = target;
	}

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct SpreadAttackReadyTransition final : Transition {
	explicit SpreadAttackReadyTransition(const AI::AIState target) { this->targetState = target; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct SpreadTelegraphFinishedTransition final : Transition {
	explicit SpreadTelegraphFinishedTransition(const AI::AIState target) { this->targetState = target; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

} // namespace Transitions

// Exposed for testing
glm::vec2 calculateRangedRayStart(Registry &registry, Entity entity, const EntityInfo &enemy, const RangedAttackComponent *ranged,
                                  const ProjectileComponent *proj, const bool facingLeft);

} // namespace GameEngine::ECS
