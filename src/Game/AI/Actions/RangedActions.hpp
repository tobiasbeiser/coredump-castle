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

} // namespace Transitions

} // namespace GameEngine::ECS
