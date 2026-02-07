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

		// Retreat distance thresholds
		// Retreat distance thresholds
		constexpr float retreatTriggerRange = 80.0f;   // Start retreating when player is this close
		constexpr float safeShootingDistance = 180.0f; // Stop retreating and resume firing at this distance

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
		    // .transition<PlayerTooCloseTransition>(AIState::PreJump, 50.0f) // JUMP PRIORITY: If very close, evade
		    // immediately
		    .transition<PlayerTooCloseTransition>(AIState::Retreat, retreatTriggerRange)
		    .transition<ProximityExitTransition>(AIState::Patrol, chase.giveUpRange)
		    .transition<LosLostTransition>(AIState::Patrol, 0.5f)

		    .addState(AIState::Retreat)
		    .onEnter<TriggerAnimationAction>(AnimationType::Roll, true)
		    .onEnter<CancelRangedAttackAction>()
		    .onUpdate<RetreatFromPlayerAction>(chase.chaseSpeed * 2.0f)
		    .onExit<SetStateComponentAction>(State::Idle) // Reset state so Fire doesn't keep rolling
		    .transition<ProximityExitTransition>(AIState::Fire, safeShootingDistance)
		    .transition<StuckTransition>(AIState::CorneredFire, 10.0f) // If stuck, go to CorneredFire (no retreat loop)
		    .transition<ProximityExitTransition>(AIState::Patrol, chase.giveUpRange)
		    .transition<LosLostTransition>(AIState::Patrol, 0.5f)

		    .addState(AIState::CorneredFire)
		    .onEnter<TriggerAnimationAction>(AnimationType::Idle, false)
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onUpdate<StopAndFacePlayerAction>()
		    .onUpdate<AimAtPlayerWithLOSAction>()
		    .onUpdate<TriggerRangedAttackAction>()
		    .onUpdate<RecordCornerAction>() // NEW: Learn where the corner is!
		    .onExit<CancelRangedAttackAction>()
		    // .transition<PlayerTooCloseTransition>(AIState::PreJump, 50.0f)           // Still jump if too close
		    .transition<ProximityExitTransition>(AIState::Patrol, chase.giveUpRange) // Give up if player leaves
		    .transition<LosLostTransition>(AIState::Patrol, 0.5f)

		    .addState(AIState::PreJump)
		    .onEnter<StopAndFaceWithAnimation>(AnimationType::Charge, true) // Telegraph animation
		    .onEnter<StartChargeTelegraphAction>()                          // Visual telegraph effect
		    .onUpdate<StopMovementAction>()
		    .onExit<StopChargeTelegraphAction>()                   // Clean up effect
		    .transition<TimerTransition>(AIState::EvadeJump, 0.5f) // Wait then jump
		    .transition<PoiseBreakTransition>(AIState::Staggered)  // Allow player to hit boss

		    .addState(AIState::EvadeJump)
		    .onEnter<TriggerAnimationAction>(AnimationType::JumpStart, false)
		    .onUpdate<EvadeJumpAction>(chase.chaseSpeed * 1.5f)
		    .onExit<CancelRangedAttackAction>()
		    .transition<LandedTransition>(AIState::Fire, 0.2f); // Wait for landing
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
		    .transition<PlayerReachableTransition>(AIState::Chase, chase.detectionRange, 32.0f)

		    .addState(AIState::Chase)
		    .onUpdate<MeleeChaseAction>(chase.chaseSpeed)
		    .transition<MeleeAttackTransition>(AIState::Attacking)
		    .transition<PlayerUnreachableTransition>(AIState::Patrol, 32.0f)
		    .transition<ProximityExitTransition>(AIState::Patrol, chase.giveUpRange)

		    .addState(AIState::Attacking)
		    .onEnter<StopAndFaceWithAnimation>(AnimationType::Attacking, false)
		    .onEnter<ExecuteAttackAction>()
		    .onUpdate<StopMovementAction>()
		    .onUpdate<FacePlayerAction>()
		    .transition<PlayerUnreachableTransition>(AIState::Patrol, 32.0f)
		    .transition<TimerTransition>(AIState::Recover, 0.8f)

		    .addState(AIState::Recover)
		    .onEnter<StopMovementAction>()
		    .onUpdate<StopMovementAction>()
		    .onUpdate<FacePlayerAction>()
		    .transition<PlayerUnreachableTransition>(AIState::Patrol, 32.0f)
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
		    .transition<PlayerReachableTransition>(AIState::Chase, chase.detectionRange, 32.0f)
		    .transition<PoiseBreakTransition>(AIState::Staggered)

		    .addState(AIState::Chase)
		    .onUpdate<MeleeChaseAction>(chase.chaseSpeed)
		    .onEnter<TriggerAnimationAction>(AnimationType::Walking, true)
		    .transition<MeleeAttackTransition>(AIState::CombatDecision)
		    .transition<isPlayerUnreachableAndGroundedTransition>(AIState::Patrol, 32.0f)
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
		    .onUpdate<StopMovementAction>()
		    .transition<ChargeTelegraphFinishedTransition>(AIState::Charging)
		    .transition<PoiseBreakTransition>(AIState::Staggered)

		    .addState(AIState::Charging)
		    .onEnter<StartChargeAction>()
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
		    .transition<isPlayerUnreachableAndGroundedTransition>(AIState::Patrol, 32.0f)
		    .transition<AnimationFinishedTransition>(AIState::Recover)
		    .transition<PoiseBreakTransition>(AIState::Staggered)

		    .addState(AIState::Recover)
		    .onEnter<StopMovementAction>()
		    .onEnter<StopAndFaceWithAnimation>(AnimationType::Idle, false)
		    .transition<isPlayerUnreachableAndGroundedTransition>(AIState::Patrol, 32.0f)
		    .transition<AnimationFinishedTransition>(AIState::Chase)
		    .transition<PoiseBreakTransition>(AIState::Staggered)

		    .addState(AIState::Staggered)
		    .onEnter<TriggerAnimationAction>(AnimationType::Hurting, false)
		    .onEnter<StopMovementAction>()
		    .onUpdate<StopMovementAction>()
		    .transition<AnimationFinishedTransition>(AIState::Chase)
		    .onExit<SetStateComponentAction>(State::Idle);
	}
};

} // namespace GameEngine::ECS
