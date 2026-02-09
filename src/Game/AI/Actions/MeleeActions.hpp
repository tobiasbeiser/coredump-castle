#pragma once

#include "CoreActions.hpp"
#include "Engine/ECS/Components/StateMachine/StateMachineComponent.hpp"

namespace GameEngine::ECS {

// Actions

namespace Actions {

struct MeleeChaseAction final : StateAction {
	float speed;
	explicit MeleeChaseAction(const float chaseSpeed) : speed(chaseSpeed) {}

	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct ExecuteAttackAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct CancelMeleeAttackAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct StartChargeAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct ChargeAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct StartChargeTelegraphAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

struct StopChargeTelegraphAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

template <typename Component, typename TimerType>
struct UpdateTimerAction final : StateAction {

	TimerType Component::*timerPtr;

	explicit UpdateTimerAction(TimerType Component::*ptr) : timerPtr(ptr) {}

	void execute(Registry &registry, const Entity entity, const float deltaTime) override
	{
		if (auto comp = registry.tryGet<Component>(entity)) {
			(comp->get().*timerPtr).step(std::chrono::duration<float>(deltaTime), false);
		}
	}
};

struct FacePlayerDuringWindupAction final : StateAction {
	void execute(Registry &registry, Entity entity, float deltaTime) override;
};

} // namespace Actions

namespace Transitions {

struct MeleeAttackTransition final : Transition {
	explicit MeleeAttackTransition(const AI::AIState target) { this->targetState = target; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct ChargeReadyTransition final : Transition {
	explicit ChargeReadyTransition(const AI::AIState targetState) { this->targetState = targetState; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct ChargeFinishedTransition final : Transition {
	explicit ChargeFinishedTransition(const AI::AIState targetState) { this->targetState = targetState; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct PoiseBreakTransition final : Transition {
	explicit PoiseBreakTransition(const AI::AIState targetState) { this->targetState = targetState; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct ChargeTelegraphFinishedTransition final : Transition {
	explicit ChargeTelegraphFinishedTransition(const AI::AIState target) { this->targetState = target; }

	bool canTransition(Registry &registry, Entity entity, float deltaTime) const override;
};

struct PlayerReachableTransition final : AndTransition {
	PlayerReachableTransition(const AI::AIState target, float radius, float height) : AndTransition(target)
	{
		add<ProximityEntryTransition>(radius);
		add<PlayerVerticalRangeTransition>(height);
	}
};

struct IsPlayerUnreachableAndGroundedTransition final : AndTransition {
	IsPlayerUnreachableAndGroundedTransition(const AI::AIState target, float maxHeight) : AndTransition(target)
	{
		add<PlayerUnreachableTransition>(maxHeight);
		add<PlayerGroundedTransition>();
	}
};

} // namespace Transitions

} // namespace GameEngine::ECS
