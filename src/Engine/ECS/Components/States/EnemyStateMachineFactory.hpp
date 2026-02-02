#pragma once

#include "Engine/ECS/Components/Gameplay/Combat/MeleeAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "StateMachineBuilder.hpp"

namespace GameEngine::ECS {

class EnemyStateMachineFactory {
  public:
	/**
	 * Adds a patrol state that transitions to combat when player is detected.
	 * Requires: PatrolComponent, ChaseComponent (for detectionRange)
	 */
	static StateMachineBuilder &addPatrolState(StateMachineBuilder &builder, Registry &registry, Entity entity,
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

	/**
	 * Adds a ranged combat state for wizard-type enemies.
	 * Requires: ChaseComponent (for giveUpRange as disengageRange)
	 */
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

	/**
	 * Create a wizard/ranged enemy (patrol -> ranged combat)
	 * Requires: PatrolComponent, ChaseComponent
	 */
	static void createWizardEnemy(Registry &registry, Entity entity)
	{
		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &chase = registry.get<ChaseComponent>(entity);

		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder builder(sm);
		addPatrolState(builder, registry, entity, "combat");
		addRangedCombatState(builder, registry, entity, "patrol");

		builder.setInitialState("patrol").build();
	}

	/**
	 * Adds a standard melee combat state that chases and attacks.
	 * Requires: ChaseComponent (for chaseSpeed, giveUpRange)
	 */
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

	/**
	 * Create a patrol-only enemy (just walks back and forth, no chase/attack)
	 * Deals contact damage only via collision, no AI combat behavior.
	 * Requires: PatrolComponent
	 */
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
		// No transitions - just patrol forever

		builder.setInitialState("patrol").build();
	}

	/**
	 * Create a melee enemy (patrol -> chase -> attack)
	 * Requires: PatrolComponent, ChaseComponent
	 */
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
		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder(sm)
		    // Idle state - deciding what to do next
		    .addState("idle")
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onUpdate<FacePlayerAction>()
		    .transition<ProximityEntryTransition>("telegraphing", 150.0f)
		    .transition<HealthThresholdTransition>("enraged", 0.5f, true) // <50% health

		    // Telegraphing - showing which attack will be used
		    .addState("telegraphing")
		    .onEnter<TriggerAnimationAction>("windup", false)
		    .onUpdate<FacePlayerAction>()
		    .withData<TelegraphStateData>("attacking", 1.0f)
		    .transition<TimerTransition>("attacking", 1.0f)
		    .transition<ProximityExitTransition>("idle", 300.0f)

		    // Attacking - execute the attack
		    .addState("attacking")
		    .onEnter<TriggerAnimationAction>("attack", false)
		    .onEnter<ExecuteAttackAction>()
		    .transition<TimerTransition>("recover", 1.5f)

		    // Charge - dash towards player
		    .addState("charge")
		    .onEnter<TriggerAnimationAction>("charge", false)
		    .onEnter<ToggleComponentAction>(ToggleComponentAction::ComponentType::Charge, true)
		    .onExit<ToggleComponentAction>(ToggleComponentAction::ComponentType::Charge, false)
		    .withData<ChargeStateData>(300.0f, 200.0f)
		    .transition<TimerTransition>("recover", 0.8f)

		    // Recover - vulnerable window
		    .addState("recover")
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onEnter<TriggerAnimationAction>("recover", false)
		    .transition<TimerTransition>("idle", 1.0f)
		    .transition<PoiseBreakTransition>("stagger")

		    // Stagger - poise broken, completely vulnerable
		    .addState("stagger")
		    .onEnter<TriggerAnimationAction>("stagger", false)
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onExit<ResetPoiseAction>()
		    .transition<TimerTransition>("recover", 3.0f)

		    // Enraged - second phase
		    .addState("enraged")
		    .onEnter<TriggerAnimationAction>("roar", false)
		    .onEnter<SetMovementSpeedAction>(150.0f) // Faster movement
		    .transition<TimerTransition>("idle", 2.0f)

		    .setInitialState("idle")
		    .build();
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

	/**
	 * Create a flying/hovering enemy state machine
	 * States: patrol, activated, chase, dive_attack, ascending
	 */
	static void createFlyingEnemy(Registry &registry, Entity entity)
	{
		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder(sm)
		    .addState("patrol")
		    .onEnter<TriggerAnimationAction>("idle", true)
		    .onUpdate<SetMovementSpeedAction>(50.0f)
		    .transition<ProximityEntryTransition>("activating", 180.0f)

		    .addState("activating")
		    .onEnter<TriggerAnimationAction>("activating", false)
		    .transition<TimerTransition>("activated", 0.5f)

		    .addState("activated")
		    .onEnter<TriggerAnimationAction>("activated", true)
		    .transition<ProximityEntryTransition>("dive_attack", 100.0f)
		    .transition<ProximityExitTransition>("deactivating", 250.0f)

		    .addState("dive_attack")
		    .onEnter<TriggerAnimationAction>("dive", false)
		    .onEnter<ExecuteAttackAction>()
		    .transition<TimerTransition>("ascending", 1.0f)

		    .addState("ascending")
		    .onEnter<TriggerAnimationAction>("ascending", false)
		    .transition<TimerTransition>("activated", 0.8f)

		    .addState("deactivating")
		    .onEnter<TriggerAnimationAction>("deactivating", false)
		    .transition<ProximityExitTransition>("patrol", 300.0f)

		    .setInitialState("patrol")
		    .build();
	}

	/**
	 * Create a heavy/tank enemy state machine
	 * States: idle, approach, block, counter_attack, vulnerable
	 */
	static void createTankEnemy(Registry &registry, Entity entity)
	{
		registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
		auto &sm = registry.get<StateMachineComponent>(entity);

		StateMachineBuilder(sm)
		    .addState("idle")
		    .onUpdate<SetVelocityAction>(glm::vec2{0, 0})
		    .transition<ProximityEntryTransition>("approach", 200.0f)

		    .addState("approach")
		    .onUpdate<MoveTowardsPlayerAction>(60.0f) // Slow movement
		    .transition<ProximityEntryTransition>("block", 80.0f)
		    .transition<ProximityExitTransition>("idle", 250.0f)

		    .addState("block")
		    .onEnter<TriggerAnimationAction>("block", true)
		    .onEnter<SetInvulnerabilityAction>(true)
		    .onExit<SetInvulnerabilityAction>(false)
		    .transition<TimerTransition>("counter_attack", 2.0f)
		    .transition<ProximityExitTransition>("approach", 120.0f)

		    .addState("counter_attack")
		    .onEnter<ExecuteAttackAction>()
		    .onEnter<TriggerAnimationAction>("heavy_attack", false)
		    .transition<TimerTransition>("vulnerable", 1.5f)

		    .addState("vulnerable")
		    .onEnter<TriggerAnimationAction>("exhausted", false)
		    .transition<TimerTransition>("idle", 2.0f) // Long recovery
		    .transition<PoiseBreakTransition>("stagger")

		    .addState("stagger")
		    .onEnter<TriggerAnimationAction>("stagger", false)
		    .onExit<ResetPoiseAction>()
		    .transition<TimerTransition>("idle", 3.0f)

		    .setInitialState("idle")
		    .build();
	}
};

} // namespace GameEngine::ECS
