#include "StateMachineSystem.hpp"
#include "Engine/ECS/Components/StateMachine/StateMachineComponent.hpp"
#include "Engine/ECS/View.hpp"
#include <SDL3/SDL_log.h>

namespace GameEngine::ECS::Systems::StateMachineSystem {

void update(Registry &registry, const float deltaTime)
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

		// Execute global update actions
		for (const auto &action : sm.globalUpdateActions) {
			action->execute(registry, entity, deltaTime);
		}

		// Get current state - and validate it explicitly
		if (sm.currentState == AI::AIState::None) {
			continue;
		}

		auto stateIt = sm.states.find(sm.currentState);
		if (stateIt == sm.states.end()) {
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "[StateMachineSystem] Entity %u has invalid state '%s'",
			            static_cast<unsigned int>(entity), AI::toString(sm.currentState).data());
			continue;
		}

		const MachineState *currentState = stateIt->second.get();

		// Execute onEnter actions if just entered
		if (sm.justEnteredState) {
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
				const AI::AIState newState = transition->targetState;

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
