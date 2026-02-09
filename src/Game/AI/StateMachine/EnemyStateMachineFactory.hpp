#pragma once

#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Utils/Timer.hpp"
#include "Game/AI/Actions/CoreActions.hpp"
#include "Game/AI/Actions/MeleeActions.hpp"
#include "Game/AI/Actions/RangedActions.hpp"
#include "StateMachineBuilder.hpp"

namespace GameEngine::ECS {

using namespace AI;
using namespace Actions;
using namespace Transitions;
class EnemyStateMachineFactory {
  public:
	static void createWizardEnemy(Registry &registry, const Entity entity)
	{
		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &chase = registry.get<ChaseComponent>(entity);

		StateMachineBuilder builder(sm);
		builder.addState(AIState::Patrol)
		    .onEnter<TriggerAnimationAction>(AnimationType::Walking, true)
		    .onUpdate<SetMovementSpeedAction>(patrol.patrolSpeed)
		    .onUpdate<PatrolAction>()
		    .transition<VisibleTargetInRangeTransition>(AIState::Fire, chase.detectionRange)

		    .addState(AIState::Fire)
		    .onEnter<TriggerAnimationAction>(AnimationType::Idle, false)
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onUpdate<StopAndFacePlayerAction>()
		    .onUpdate<AimAtPlayerWithLOSAction>()
		    .onUpdate<TriggerRangedAttackAction>()
		    .onExit<CancelRangedAttackAction>()
		    .transition<ProximityExitTransition>(AIState::Patrol, chase.giveUpRange)
		    .transition<LosLostTransition>(AIState::Patrol, 0.5f);
	}

	static void createArcherBoss(Registry &registry, const Entity entity)
	{
		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &chase = registry.get<ChaseComponent>(entity);

		constexpr float retreatTriggerRange = 130.0f;
		constexpr float escapeTriggerRange = 80.0f; // "Too close" range
		constexpr float safeShootingDistance = 280.0f;

		float giveUpRange = std::max(chase.giveUpRange, safeShootingDistance + 100.0f);

		StateMachineBuilder builder(sm);
		builder.addState(AIState::Patrol)
		    .onEnter<TriggerAnimationAction>(AnimationType::Walking, true)
		    .onUpdate<SetMovementSpeedAction>(patrol.patrolSpeed)
		    .onUpdate<PatrolAction>()
		    .transition<VisibleTargetInRangeTransition>(AIState::Fire, chase.detectionRange)

		    .addState(AIState::Fire)
		    .onEnter<TriggerAnimationAction>(AnimationType::Idle, false)
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onUpdate<StopAndFacePlayerAction>()
		    .onUpdate<AimAtPlayerWithLOSAction>()
		    .onUpdate<TriggerRangedAttackAction>()
		    .onExit<CancelRangedAttackAction>()
		    .transition<PlayerTooCloseTransition>(AIState::TelegraphCharge, escapeTriggerRange)
		    .transition<PlayerTooCloseTransition>(AIState::Retreat, retreatTriggerRange)
		    .transition<ProximityExitTransition>(AIState::Patrol, giveUpRange)
		    .transition<LosLostTransition>(AIState::Patrol, 0.5f)

		    .addState(AIState::Retreat)
		    .onEnter<TriggerAnimationAction>(AnimationType::Roll, true)
		    .onEnter<CancelRangedAttackAction>()
		    .onUpdate<RetreatFromPlayerAction>(220)
		    .onExit<SetStateComponentAction>(State::Idle)
		    .transition<ProximityExitTransition>(AIState::Fire, safeShootingDistance)
		    .transition<StuckTransition>(AIState::CorneredFire, 20.0f, 0.1f)
		    .transition<ProximityExitTransition>(AIState::Patrol, giveUpRange)
		    .transition<LosLostTransition>(AIState::Patrol, 0.5f)

		    .addState(AIState::CorneredFire)
		    .onEnter<TriggerAnimationAction>(AnimationType::Idle, false)
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onUpdate<StopAndFacePlayerAction>()
		    .onUpdate<AimAtPlayerWithLOSAction>()
		    .onUpdate<TriggerRangedAttackAction>()
		    .onUpdate<RecordCornerAction>()
		    .onExit<CancelRangedAttackAction>()
		    .transition<PlayerTooCloseTransition>(AIState::TelegraphCharge, escapeTriggerRange)
		    .transition<CanRetreatTransition>(AIState::Fire)
		    .transition<SafeDistanceReachedTransition>(AIState::Fire, safeShootingDistance + 50.0f)
		    .transition<LosLostTransition>(AIState::Patrol, 0.5f)

		    .addState(AIState::TelegraphCharge)
		    .onEnter<StopAndFaceWithAnimation>(AnimationType::Interacting, true)
		    .onEnter<StartChargeTelegraphAction>()
		    .onUpdate<StopMovementAction>()
		    .onExit<StopChargeTelegraphAction>()
		    .transition<TimerTransition>(AIState::Escaping, 1.2f)

		    .addState(AIState::Escaping)
		    .onEnter<TriggerAnimationAction>(AnimationType::Walking, true)
		    .onEnter<PhaseShiftAction>()
		    .onUpdate<ApplyCameraShakeAction>(7.0f, 1.2f)
		    .onUpdate<ApplyProximitySlowAction>(160.0f, 0.22f, 1.3f)
		    .onUpdate<ApplyConcussionVignetteAction>(160.0f, 1.8f)
		    .onUpdate<EscapeThroughPlayerAction>(200)
		    .onExit<RestoreOpacityAction>()
		    .transition<SafePassedPlayerTransition>(AIState::Retreat, 80.0f)
		    .transition<TimerTransition>(AIState::Retreat, 2.0f);
	}

	static void createPatrolOnlyEnemy(Registry &registry, const Entity entity)
	{
		const auto &patrol = registry.get<PatrolComponent>(entity);

		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder builder(sm);

		builder.addState(AIState::Patrol)
		    .onEnter<TriggerAnimationAction>(AnimationType::Walking, true)
		    .onUpdate<SetMovementSpeedAction>(patrol.patrolSpeed)
		    .onUpdate<PatrolAction>();
	}

	static void createMeleeEnemy(Registry &registry, Entity entity)
	{
		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &chase = registry.get<ChaseComponent>(entity);

		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder builder(sm);

		builder.addState(AIState::Patrol)
		    .onEnter<CancelMeleeAttackAction>()
		    .onEnter<TriggerAnimationAction>(AnimationType::Walking, true)
		    .onUpdate<SetMovementSpeedAction>(patrol.patrolSpeed)
		    .onUpdate<PatrolAction>()
		    .transition<PlayerReachableTransition>(AIState::Chase, chase.detectionRange, 10.0f)

		    .addState(AIState::Chase)
		    .onUpdate<MeleeChaseAction>(chase.chaseSpeed)
		    .transition<MeleeAttackTransition>(AIState::Attacking)
		    .transition<PlayerUnreachableTransition>(AIState::Patrol, 10.0f)
		    .transition<ProximityExitTransition>(AIState::Patrol, chase.giveUpRange)

		    .addState(AIState::Attacking)
		    .onEnter<StopAndFaceWithAnimation>(AnimationType::Attacking, false)
		    .onEnter<ExecuteAttackAction>()
		    .onUpdate<StopMovementAction>()
		    .onUpdate<FacePlayerAction>()
		    .transition<PlayerUnreachableTransition>(AIState::Patrol, 10.0f)
		    .transition<TimerTransition>(AIState::Recover, 0.8f)

		    .addState(AIState::Recover)
		    .onEnter<StopMovementAction>()
		    .onUpdate<StopMovementAction>()
		    .onUpdate<FacePlayerAction>()
		    .transition<PlayerUnreachableTransition>(AIState::Patrol, 10.0f)
		    .transition<TimerTransition>(AIState::Chase, 0.5f);
	}

	static void createBossEnemy(Registry &registry, Entity entity)
	{
		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &chase = registry.get<ChaseComponent>(entity);

		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder builder(sm);
		builder.addGlobalUpdate<UpdateTimerAction<ChargeComponent, Timer>>(&ChargeComponent::chargeCooldown);

		builder.addState(AIState::Patrol)
		    .onEnter<CancelMeleeAttackAction>()
		    .onEnter<TriggerAnimationAction>(AnimationType::Walking, true)
		    .onUpdate<SetMovementSpeedAction>(patrol.patrolSpeed)
		    .onUpdate<PatrolAction>()
		    .transition<PlayerReachableTransition>(AIState::Chase, chase.detectionRange, 10.0f)
		    .transition<PoiseBreakTransition>(AIState::Staggered)

		    .addState(AIState::Chase)
		    .onUpdate<MeleeChaseAction>(chase.chaseSpeed)
		    .onEnter<TriggerAnimationAction>(AnimationType::Walking, true)
		    .transition<MeleeAttackTransition>(AIState::CombatDecision)
		    .transition<IsPlayerUnreachableAndGroundedTransition>(AIState::Patrol, 10.0f)
		    .transition<ProximityExitTransition>(AIState::Patrol, chase.giveUpRange)
		    .transition<PoiseBreakTransition>(AIState::Staggered)

		    .addState(AIState::CombatDecision)
		    .transition<ChargeReadyTransition>(AIState::CheckChargeChance)
		    .transition<ImmediateTransition>(AIState::Attacking)

		    .addState(AIState::CheckChargeChance)
		    .transition<RandomTransition>(AIState::TelegraphCharge, 0.9f)
		    .transition<ImmediateTransition>(AIState::Attacking)

		    .addState(AIState::TelegraphCharge)
		    .onEnter<StartChargeTelegraphAction>()
		    .onEnter<StopAndFaceWithAnimation>(AnimationType::Idle, true)
		    .onEnter<ApplyCameraShakeAction>(6.0f, 0.25f)
		    .onUpdate<StopMovementAction>()
		    .transition<ChargeTelegraphFinishedTransition>(AIState::Charging)
		    .transition<PoiseBreakTransition>(AIState::Staggered)

		    .addState(AIState::TelegraphEscape)
		    .onEnter<StopAndFaceWithAnimation>(AnimationType::Idle, true)
		    .onUpdate<StopMovementAction>()
		    .onExit<StopChargeTelegraphAction>()
		    .transition<TimerTransition>(AIState::Attacking, 0.35f)

		    .addState(AIState::Charging)
		    .onEnter<StartChargeAction>()
		    .onEnter<ApplyCameraShakeAction>(12.0f, 0.5f)
		    .onEnter<TriggerAnimationAction>(AnimationType::Walking, true)
		    .onUpdate<ChargeAction>()
		    .transition<ChargeFinishedTransition>(AIState::Recover)
		    .transition<PoiseBreakTransition>(AIState::Staggered)

		    .addState(AIState::Attacking)
		    .onEnter<StopAndFaceWithAnimation>(AnimationType::Attacking, false)
		    .onEnter<ExecuteAttackAction>()
		    .onUpdate<StopMovementAction>()
		    .onUpdate<FacePlayerDuringWindupAction>()
		    .onExit<CancelMeleeAttackAction>()
		    .transition<IsPlayerUnreachableAndGroundedTransition>(AIState::Patrol, 10.0f)
		    .transition<AnimationFinishedTransition>(AIState::Recover)
		    .transition<PoiseBreakTransition>(AIState::Staggered)

		    .addState(AIState::Recover)
		    .onEnter<StopMovementAction>()
		    .onEnter<StopAndFaceWithAnimation>(AnimationType::Idle, false)
		    .transition<IsPlayerUnreachableAndGroundedTransition>(AIState::Patrol, 10.0f)
		    .transition<AnimationFinishedTransition>(AIState::Chase)
		    .transition<PoiseBreakTransition>(AIState::Staggered)
		    .addState(AIState::Staggered)
		    .onEnter<TriggerAnimationAction>(AnimationType::Hurting, false)
		    .onEnter<StopMovementAction>()
		    .onUpdate<StopMovementAction>()
		    .transition<TimerTransition>(AIState::Chase, 1.2f)
		    .onExit<SetStateComponentAction>(State::Idle);
	}
};

} // namespace GameEngine::ECS
