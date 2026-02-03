#include "StateMachineSystem.hpp"
#include "Engine/ECS/Components/States/StateMachineComponent.hpp"
#include "Engine/ECS/View.hpp"
#include <SDL3/SDL_log.h>

namespace GameEngine::ECS::Systems::StateMachineSystem {

void update(Registry &registry, float deltaTime)
{
	auto view = registry.view<StateMachineComponent>();

	static bool firstRun = true;
	if (firstRun) {
		int count = 0;
		for (const auto entity : view) {
			count++;
		}
		SDL_Log("[StateMachineSystem] First update: %d entities with state machines", count);
		firstRun = false;
	}

	for (const auto entity : view) {
		const auto smOpt = registry.tryGet<StateMachineComponent>(entity);
		if (!smOpt)
			continue;

		StateMachineComponent &sm = smOpt->get();

		// Update timers
		sm.timeInState += deltaTime;

		// Get current state
		if (sm.currentState.empty() || sm.states.find(sm.currentState) == sm.states.end()) {
			continue;
		}

		const MachineState *currentState = sm.states[sm.currentState].get();

		// Execute onEnter actions if just entered
		if (sm.justEnteredState) {
			SDL_Log("[StateMachine] Entity %u entering state '%s', executing %zu onEnter actions",
			        static_cast<unsigned int>(entity), sm.currentState.c_str(), currentState->onEnterActions.size());
			for (const auto &action : currentState->onEnterActions) {
				action->execute(registry, entity, deltaTime);
			}
			sm.justEnteredState = false;
		}

		// Execute onUpdate actions
		for (const auto &action : currentState->onUpdateActions) {
			action->execute(registry, entity, deltaTime);
		}

		// Check for transitions
		for (const auto &transition : currentState->transitions) {
			if (transition->canTransition(registry, entity, deltaTime)) {
				// Found valid transition - change state
				const std::string newState = transition->targetState;

				// Execute onExit actions
				for (const auto &action : currentState->onExitActions) {
					action->execute(registry, entity, deltaTime);
				}

				// Change state
				sm.previousState = sm.currentState;
				sm.currentState = newState;
				sm.timeInState = 0.0f;
				sm.justEnteredState = true;

				// Don't check more transitions - we've already changed state
				break;
			}
		}
	}
}

} // namespace GameEngine::ECS::Systems::StateMachineSystem
