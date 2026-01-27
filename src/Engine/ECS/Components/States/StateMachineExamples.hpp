#pragma once

#include "Engine/ECS/Components/States/StateMachineBuilder.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Examples {

/**
 * Example usage of the state machine system for various enemy types
 */

// ============================================================================
// Example 1: Simple Skeleton Enemy
// ============================================================================
void createSkeletonEnemy(Registry &registry, Entity entity)
{
	auto &sm = registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});

	StateMachineBuilder(sm)
	    // Patrolling - just walking back and forth
	    .addState("patrol")
	    .onUpdate<SetVelocityAction>(glm::vec2{0, 0})
	    .transition<ProximityEntryTransition>("chase", 180.0f)

	    // Chase - run towards player
	    .addState("chase")
	    .onUpdate<MoveTowardsPlayerAction>(120.0f)
	    .transition<ProximityEntryTransition>("attack", 60.0f)
	    .transition<ProximityExitTransition>("patrol", 220.0f)

	    // Attack - simple melee
	    .addState("attack")
	    .onEnter<ExecuteAttackAction>()
	    .onUpdate<FacePlayerAction>()
	    .transition<TimerTransition>("chase", 1.0f)

	    .setInitialState("patrol")
	    .build();
}

// ============================================================================
// Example 2: Dragon Boss with Multiple Phases
// ============================================================================
void createDragonBoss(Registry &registry, Entity entity)
{
	auto &sm = registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});

	StateMachineBuilder(sm)
	    // Idle - waiting and observing
	    .addState("idle")
	    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
	    .onUpdate<FacePlayerAction>()
	    .transition<TimerTransition>("choose_attack", 2.0f)

	    // Decision state - AI decides which attack to use
	    .addState("choose_attack")
	    .onUpdate<FacePlayerAction>()
	    .transition<RandomTransition>("fire_breath_telegraph", 0.4f) // 40% fire
	    .transition<RandomTransition>("tail_swipe_telegraph", 0.3f)  // 30% tail
	    .transition<RandomTransition>("fly_up", 0.3f)                // 30% fly

	    // === FIRE BREATH ATTACK ===
	    .addState("fire_breath_telegraph")
	    .onEnter<TriggerAnimationAction>("fire_windup", false)
	    .onUpdate<FacePlayerAction>()
	    .transition<TimerTransition>("fire_breath", 1.5f)

	    .addState("fire_breath")
	    .onEnter<TriggerAnimationAction>("fire_attack", false)
	    .onEnter<ExecuteAttackAction>()
	    .transition<TimerTransition>("recover", 2.0f)

	    // === TAIL SWIPE ATTACK ===
	    .addState("tail_swipe_telegraph")
	    .onEnter<TriggerAnimationAction>("tail_windup", false)
	    .transition<TimerTransition>("tail_swipe", 0.8f)

	    .addState("tail_swipe")
	    .onEnter<TriggerAnimationAction>("tail_attack", false)
	    .onEnter<ExecuteAttackAction>()
	    .transition<TimerTransition>("recover", 1.5f)

	    // === AERIAL ATTACK ===
	    .addState("fly_up")
	    .onEnter<TriggerAnimationAction>("takeoff", false)
	    .onEnter<ToggleComponentAction>(ToggleComponentAction::ComponentType::Gravity, false)
	    .transition<TimerTransition>("aerial", 1.0f)

	    .addState("aerial")
	    .onEnter<TriggerAnimationAction>("flying", true)
	    .transition<TimerTransition>("dive", 2.0f)

	    .addState("dive")
	    .onEnter<TriggerAnimationAction>("dive_attack", false)
	    .onEnter<ExecuteAttackAction>()
	    .transition<TimerTransition>("landing", 1.0f)

	    .addState("landing")
	    .onEnter<TriggerAnimationAction>("land", false)
	    .onExit<ToggleComponentAction>(ToggleComponentAction::ComponentType::Gravity, true)
	    .transition<TimerTransition>("recover", 0.8f)

	    // === RECOVERY & PHASE TRANSITIONS ===
	    .addState("recover")
	    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
	    .transition<TimerTransition>("idle", 1.0f)
	    .transition<PoiseBreakTransition>("stagger")
	    .transition<HealthThresholdTransition>("phase_2", 0.7f, true) // <70%

	    .addState("stagger")
	    .onEnter<TriggerAnimationAction>("stagger", false)
	    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
	    .onExit<ResetPoiseAction>()
	    .transition<TimerTransition>("recover", 3.0f)

	    // Phase 2 - faster and more aggressive
	    .addState("phase_2")
	    .onEnter<TriggerAnimationAction>("roar", false)
	    .onEnter<SetMovementSpeedAction>(150.0f) // Faster
	    .transition<TimerTransition>("idle", 2.0f)

	    .setInitialState("idle")
	    .build();
}

// ============================================================================
// Example 3: Archer Enemy (Ranged with Kiting)
// ============================================================================
void createArcherEnemy(Registry &registry, Entity entity)
{
	auto &sm = registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});

	StateMachineBuilder(sm)
	    .addState("patrol")
	    .onUpdate<SetMovementSpeedAction>(60.0f)
	    .transition<ProximityEntryTransition>("engage", 250.0f)

	    .addState("engage")
	    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
	    .onUpdate<FacePlayerAction>()
	    .transition<ProximityExitTransition>("patrol", 300.0f)
	    .transition<ProximityEntryTransition>("retreat", 80.0f) // Player too close
	    .transition<TimerTransition>("aim", 0.5f)

	    .addState("aim")
	    .onEnter<TriggerAnimationAction>("draw_bow", false)
	    .onUpdate<FacePlayerAction>()
	    .transition<TimerTransition>("shoot", 0.8f)
	    .transition<ProximityEntryTransition>("retreat", 70.0f)

	    .addState("shoot")
	    .onEnter<ExecuteAttackAction>()
	    .onEnter<TriggerAnimationAction>("shoot", false)
	    .transition<TimerTransition>("engage", 1.2f)

	    .addState("retreat")
	    .onEnter<TriggerAnimationAction>("run", true)
	    .onUpdate<SetMovementSpeedAction>(140.0f) // Fast retreat
	    .transition<ProximityExitTransition>("engage", 120.0f)
	    .transition<TimerTransition>("engage", 3.0f) // Give up after 3s

	    .setInitialState("patrol")
	    .build();
}

// ============================================================================
// Example 4: Mini-Boss: Armored Knight
// ============================================================================
void createArmoredKnight(Registry &registry, Entity entity)
{
	auto &sm = registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});

	StateMachineBuilder(sm)
	    .addState("idle")
	    .onUpdate<SetVelocityAction>(glm::vec2{0, 0})
	    .transition<ProximityEntryTransition>("approach", 200.0f)

	    .addState("approach")
	    .onUpdate<MoveTowardsPlayerAction>(70.0f) // Slow, heavy
	    .transition<ProximityEntryTransition>("shield_up", 90.0f)
	    .transition<ProximityExitTransition>("idle", 230.0f)

	    .addState("shield_up")
	    .onEnter<TriggerAnimationAction>("shield_block", true)
	    .onEnter<SetInvulnerabilityAction>(true) // Can't be damaged
	    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
	    .onUpdate<FacePlayerAction>()
	    .onExit<SetInvulnerabilityAction>(false)
	    .transition<TimerTransition>("shield_bash", 1.5f)
	    .transition<ProximityExitTransition>("approach", 130.0f)

	    .addState("shield_bash")
	    .onEnter<TriggerAnimationAction>("bash", false)
	    .onEnter<ExecuteAttackAction>()
	    .transition<TimerTransition>("combo_decide", 1.0f)

	    .addState("combo_decide")
	    .onUpdate<FacePlayerAction>()
	    .transition<RandomTransition>("overhead_strike", 0.6f) // 60% overhead
	    .transition<RandomTransition>("vulnerable", 0.4f)      // 40% tired

	    .addState("overhead_strike")
	    .onEnter<TriggerAnimationAction>("overhead_windup", false)
	    .transition<TimerTransition>("overhead_execute", 0.8f)

	    .addState("overhead_execute")
	    .onEnter<TriggerAnimationAction>("overhead_attack", false)
	    .onEnter<ExecuteAttackAction>()
	    .transition<TimerTransition>("vulnerable", 1.5f)

	    .addState("vulnerable")
	    .onEnter<TriggerAnimationAction>("tired", false)
	    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
	    .transition<TimerTransition>("idle", 2.5f) // Long recovery
	    .transition<PoiseBreakTransition>("stagger")

	    .addState("stagger")
	    .onEnter<TriggerAnimationAction>("stagger", false)
	    .onExit<ResetPoiseAction>()
	    .transition<TimerTransition>("idle", 3.0f)

	    .setInitialState("idle")
	    .build();
}

// ============================================================================
// Example 5: Bat/Flying Enemy (Swooping)
// ============================================================================
void createBatEnemy(Registry &registry, Entity entity)
{
	auto &sm = registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});

	StateMachineBuilder(sm)
	    .addState("sleeping")
	    .onEnter<TriggerAnimationAction>("sleep", true)
	    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
	    .transition<ProximityEntryTransition>("waking", 100.0f)

	    .addState("waking")
	    .onEnter<TriggerAnimationAction>("wake", false)
	    .transition<TimerTransition>("hover", 0.5f)

	    .addState("hover")
	    .onEnter<TriggerAnimationAction>("fly", true)
	    .onUpdate<FacePlayerAction>()
	    .transition<ProximityEntryTransition>("swoop_telegraph", 80.0f)
	    .transition<ProximityExitTransition>("sleeping", 150.0f)

	    .addState("swoop_telegraph")
	    .onEnter<TriggerAnimationAction>("prepare_swoop", false)
	    .onUpdate<FacePlayerAction>()
	    .transition<TimerTransition>("swooping", 0.3f)

	    .addState("swooping")
	    .onEnter<TriggerAnimationAction>("swoop", false)
	    .onEnter<ExecuteAttackAction>()
	    .onUpdate<MoveTowardsPlayerAction>(200.0f) // Fast dive
	    .transition<TimerTransition>("ascending", 0.6f)

	    .addState("ascending")
	    .onEnter<TriggerAnimationAction>("fly_up", false)
	    .transition<TimerTransition>("hover", 0.8f)

	    .setInitialState("sleeping")
	    .build();
}

// ============================================================================
// Example 6: Slime Enemy (Simple but with Split Mechanic)
// ============================================================================
void createSlimeEnemy(Registry &registry, Entity entity, bool isSmall = false)
{
	auto &sm = registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});

	if (!isSmall) {
		// Large slime - can split
		StateMachineBuilder(sm)
		    .addState("idle")
		    .onUpdate<SetVelocityAction>(glm::vec2{0, 0})
		    .transition<ProximityEntryTransition>("hop_towards", 150.0f)

		    .addState("hop_towards")
		    .onEnter<TriggerAnimationAction>("hop", false)
		    .onUpdate<MoveTowardsPlayerAction>(90.0f)
		    .transition<ProximityEntryTransition>("attack", 50.0f)
		    .transition<ProximityExitTransition>("idle", 180.0f)

		    .addState("attack")
		    .onEnter<TriggerAnimationAction>("slam", false)
		    .onEnter<ExecuteAttackAction>()
		    .transition<TimerTransition>("hop_towards", 1.0f)
		    .transition<HealthThresholdTransition>("splitting", 0.3f, true) // <30%

		    .addState("splitting")
		    .onEnter<TriggerAnimationAction>("split", false)
		    // Note: Would spawn two small slimes here in a system
		    .transition<TimerTransition>("dead", 0.5f)

		    .addState("dead")
		    // Entity marked for removal

		    .setInitialState("idle")
		    .build();
	} else {
		// Small slime - simpler behavior
		StateMachineBuilder(sm)
		    .addState("idle")
		    .transition<ProximityEntryTransition>("chase", 120.0f)

		    .addState("chase")
		    .onUpdate<MoveTowardsPlayerAction>(110.0f)
		    .transition<ProximityEntryTransition>("attack", 40.0f)
		    .transition<ProximityExitTransition>("idle", 150.0f)

		    .addState("attack")
		    .onEnter<ExecuteAttackAction>()
		    .transition<TimerTransition>("chase", 0.8f)

		    .setInitialState("idle")
		    .build();
	}
}

} // namespace GameEngine::ECS::Examples
