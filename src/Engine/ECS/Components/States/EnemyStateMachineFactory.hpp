#pragma once

#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "StateMachineBuilder.hpp"

namespace GameEngine::ECS {

class EnemyStateMachineFactory {
  public:
	static StateMachineBuilder &addPatrolState(StateMachineBuilder &builder, Registry &registry, const Entity entity,
	                                           const std::string &nextState = "combat")
	{
		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &chase = registry.get<ChaseComponent>(entity);

		return builder.addState("patrol")
		    .onEnter<TriggerAnimationAction>("walk", true)
		    .onUpdate<SetMovementSpeedAction>(patrol.patrolSpeed)
		    .onUpdate<PatrolAction>()
		    .transition<ProximityAndLosEntryTransition>(nextState, chase.detectionRange);
	}

	static StateMachineBuilder &addRangedCombatState(StateMachineBuilder &builder, Registry &registry, Entity entity,
	                                                 const std::string &returnState = "patrol")
	{
		const auto &chase = registry.get<ChaseComponent>(entity);

		return builder.addState("combat")
		    .onEnter<TriggerAnimationAction>("idle", false)
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onUpdate<StopAndFacePlayerAction>()
		    .onUpdate<AimAtPlayerWithLOSAction>()
		    .onUpdate<TriggerRangedAttackAction>()
		    .onExit<CancelRangedAttackAction>()
		    .transition<ProximityExitTransition>(returnState, chase.giveUpRange)
		    .transition<LosLostTransition>(returnState, 0.5f);
	}

	static void createWizardEnemy(Registry &registry, const Entity entity)
	{
		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder builder(sm);
		addPatrolState(builder, registry, entity, "combat");
		addRangedCombatState(builder, registry, entity, "patrol");

		builder.setInitialState("patrol").build();
	}

	static StateMachineBuilder &addMeleeCombatState(StateMachineBuilder &builder, Registry &registry, Entity entity,
	                                                const std::string &returnState = "patrol")
	{
		const auto &chase = registry.get<ChaseComponent>(entity);

		return builder.addState("chase")
		    .onUpdate<MeleeChaseAction>(chase.chaseSpeed)
		    .transition<MeleeAttackTransition>("attacking")
		    .transition<PlayerUnreachableTransition>(returnState, 32.0f)
		    .transition<ProximityExitTransition>(returnState, chase.giveUpRange)

		    .addState("attacking")
		    .onEnter<TriggerAnimationAction>("attack", false)
		    .onEnter<StopMovementAction>()
		    .onEnter<FacePlayerAction>()
		    .onEnter<ExecuteAttackAction>()
		    .onUpdate<StopMovementAction>()
		    .onUpdate<FacePlayerAction>()
		    .transition<PlayerUnreachableTransition>(returnState, 32.0f)
		    .transition<TimerTransition>("recover", 0.8f)

		    .addState("recover")
		    .onEnter<StopMovementAction>()
		    .onUpdate<StopMovementAction>()
		    .onUpdate<FacePlayerAction>()
		    .transition<PlayerUnreachableTransition>(returnState, 32.0f)
		    .transition<TimerTransition>("chase", 0.5f); // Go back to chase to re-evaluate
	}

	static void createPatrolOnlyEnemy(Registry &registry, Entity entity)
	{
		const auto &patrol = registry.get<PatrolComponent>(entity);

		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder builder(sm);

		builder.addState("patrol")
		    .onEnter<TriggerAnimationAction>("walk", true)
		    .onUpdate<SetMovementSpeedAction>(patrol.patrolSpeed)
		    .onUpdate<PatrolAction>();

		builder.setInitialState("patrol").build();
	}

	static void createMeleeEnemy(Registry &registry, Entity entity)
	{
		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &chase = registry.get<ChaseComponent>(entity);

		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder builder(sm);

		builder.addState("patrol")
		    .onEnter<CancelMeleeAttackAction>()
		    .onEnter<TriggerAnimationAction>("walk", true)
		    .onUpdate<SetMovementSpeedAction>(patrol.patrolSpeed)
		    .onUpdate<PatrolAction>()
		    .transition<MeleeProximityEntryTransition>("chase", chase.detectionRange, 32.0f);

		addMeleeCombatState(builder, registry, entity, "patrol");

		builder.setInitialState("patrol").build();
	}

	static void createBossEnemy(Registry &registry, Entity entity)
	{
		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &chase = registry.get<ChaseComponent>(entity);

		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder builder(sm);

		builder.addState("patrol")
		    .onEnter<CancelMeleeAttackAction>()
		    .onEnter<TriggerAnimationAction>("walk", true)
		    .onUpdate<SetMovementSpeedAction>(patrol.patrolSpeed)
		    .onUpdate<PatrolAction>()
		    .transition<MeleeProximityEntryTransition>("chase", chase.detectionRange, 32.0f)
		    .transition<PoiseBreakTransition>("staggered");

		builder.addState("chase")
		    .onUpdate<MeleeChaseAction>(chase.chaseSpeed)
		    .onEnter<TriggerAnimationAction>("walk", true)
		    .transition<MeleeAttackTransition>("attacking")
		    .transition<PlayerUnreachableAndGroundedTransition>("patrol", 32.0f)
		    .transition<ProximityExitTransition>("patrol", chase.giveUpRange)
		    .transition<PoiseBreakTransition>("staggered")

		    .addState("attacking")
		    .onEnter<TriggerAnimationAction>("attack", false)
		    .onEnter<StopMovementAction>()
		    .onEnter<FacePlayerAction>()
		    .onEnter<ExecuteAttackAction>()
		    .onUpdate<StopMovementAction>()
		    .onUpdate<FacePlayerDuringWindupAction>()
		    .onExit<CancelMeleeAttackAction>()
		    .transition<PlayerUnreachableAndGroundedTransition>("patrol", 32.0f)
		    .transition<AnimationFinishedTransition>("recover")
		    .transition<PoiseBreakTransition>("staggered")

		    .addState("recover")
		    .onEnter<StopMovementAction>()
		    .onEnter<TriggerAnimationAction>("idle", true)
		    .onUpdate<StopMovementAction>()
		    .onUpdate<FacePlayerAction>()
		    .transition<PlayerUnreachableAndGroundedTransition>("patrol", 32.0f)
		    .transition<TimerTransition>("chase", 0.1f)
		    .transition<PoiseBreakTransition>("staggered")

		    .addState("staggered")
		    .onEnter<TriggerAnimationAction>("hurting", false)
		    .onEnter<StopMovementAction>()
		    .onUpdate<StopMovementAction>()
		    .transition<AnimationFinishedTransition>("chase")
		    .onExit<SetStateComponentAction>(State::Idle);

		builder.setInitialState("patrol").build();
	}

	static void createRangedEnemy(Registry &registry, Entity entity)
	{
		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder(sm)
		    .addState("idle")
		    .onUpdate<SetVelocityAction>(glm::vec2{0, 0})
		    .transition<ProximityEntryTransition>("chase", 300.0f)

		    .addState("chase")
		    .onUpdate<MoveTowardsPlayerAction>(80.0f)
		    .transition<ProximityEntryTransition>("aiming", 150.0f)
		    .transition<ProximityExitTransition>("idle", 350.0f)

		    .addState("aiming")
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onUpdate<FacePlayerAction>()
		    .transition<TimerTransition>("shooting", 0.5f)
		    .transition<ProximityEntryTransition>("retreat", 50.0f) // Too close

		    .addState("shooting")
		    .onEnter<ExecuteAttackAction>()
		    .transition<TimerTransition>("idle", 1.0f)

		    .addState("retreat")
		    .onUpdate<SetMovementSpeedAction>(120.0f) // Move away fast
		    .transition<ProximityExitTransition>("aiming", 100.0f)
		    .transition<TimerTransition>("idle", 2.0f)

		    .setInitialState("idle")
		    .build();
	}
};

} // namespace GameEngine::ECS
