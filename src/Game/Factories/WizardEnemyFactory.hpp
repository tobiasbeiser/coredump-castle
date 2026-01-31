#pragma once

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/States/StateMachineBuilder.hpp"
#include "Engine/ECS/Components/States/StateTransitionsAndActions.hpp"
#include "Engine/ECS/Entity.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
#include <SDL3/SDL_log.h>

namespace GameEngine::ECS {

// Configuration for wizard state machine (passed from Tiled properties)
struct WizardConfig {
	float patrolSpeed = 80.0f;
	float detectionRange = 300.0f; // Range to detect player and enter combat
	float disengageRange = 350.0f; // Range to lose player and return to patrol
};

class WizardEnemyFactory {
  public:
	static void addStateMachine(Registry &registry, Entity wizard, const WizardConfig &config = {})
	{
		SDL_Log("[WizardFactory] Adding state machine to entity %u", static_cast<unsigned int>(wizard));
		createStateMachine(registry, wizard, config);
	}

  private:
	static void createStateMachine(Registry &registry, Entity wizard, const WizardConfig &config)
	{
		SDL_Log("[WizardFactory] Registering StateMachineComponent for entity %u", static_cast<unsigned int>(wizard));
		registry.registerComponent<StateMachineComponent>(wizard, StateMachineComponent{});
		SDL_Log("[WizardFactory] Component registered successfully");
		auto smOpt = registry.tryGet<StateMachineComponent>(wizard);
		if (!smOpt)
			return;

		StateMachineComponent &sm = smOpt->get();

		StateMachineBuilder(sm)
		    .addState("patrol")
		    .onEnter<TriggerAnimationAction>("walk", true)
		    .onUpdate<SetMovementSpeedAction>(config.patrolSpeed)
		    .onUpdate<PatrolAction>()                                                    // Use core system for movement
		    .transition<ProximityAndLosEntryTransition>("combat", config.detectionRange) // Requires LOS

		    .addState("combat")
		    .onEnter<TriggerAnimationAction>("idle", false)
		    .onEnter<SetVelocityAction>(glm::vec2{0, 0})
		    .onUpdate<RangedAttackAction>()
		    .onExit<CancelRangedAttackAction>()
		    .transition<ProximityExitTransition>("patrol", config.disengageRange)
		    .transition<LosLostTransition>("patrol", 0.5f) // Return to patrol if LOS lost for 0.5s

		    .setInitialState("patrol")
		    .build();

		SDL_Log("[WizardFactory] State machine configured (detectRange=%.1f, disengageRange=%.1f, patrolSpeed=%.1f)",
		        config.detectionRange, config.disengageRange, config.patrolSpeed);
	}
};

} // namespace GameEngine::ECS
