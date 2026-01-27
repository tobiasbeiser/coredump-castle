#pragma once

#include "Engine/ECS/Entity.hpp"
#include "Engine/ECS/Registry.hpp"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace GameEngine::ECS {

// Forward declaration
class Registry;

// Base class for all state data
struct StateData {
	virtual ~StateData() = default;
};

// Base class for all transitions
struct Transition {
	std::string targetState;

	virtual ~Transition() = default;
	virtual bool canTransition(Registry &registry, Entity entity, float deltaTime) const = 0;
};

// Base class for all actions
struct StateAction {
	virtual ~StateAction() = default;
	virtual void execute(Registry &registry, Entity entity, float deltaTime) = 0;
};

// Represents a single state in the state machine
struct MachineState {
	std::string name;

	// Actions to perform on state entry
	std::vector<std::unique_ptr<StateAction>> onEnterActions;

	// Actions to perform while in this state
	std::vector<std::unique_ptr<StateAction>> onUpdateActions;

	// Actions to perform on state exit
	std::vector<std::unique_ptr<StateAction>> onExitActions;

	// Possible transitions from this state
	std::vector<std::unique_ptr<Transition>> transitions;

	// Optional custom state data
	std::unique_ptr<StateData> data;

	explicit MachineState(std::string stateName) : name(std::move(stateName)) {}
};

// The state machine component
struct StateMachineComponent {
	// All available states
	std::unordered_map<std::string, std::unique_ptr<MachineState>> states;

	// Current active state
	std::string currentState;

	// Previous state (for reference)
	std::string previousState;

	// Time spent in current state
	float timeInState = 0.0f;

	// Flag to track if we're in the first frame of a new state
	bool justEnteredState = false;

	StateMachineComponent() = default;
	StateMachineComponent(StateMachineComponent &&) = default;
	StateMachineComponent &operator=(StateMachineComponent &&) = default;
	StateMachineComponent(const StateMachineComponent &) = delete;
	StateMachineComponent &operator=(const StateMachineComponent &) = delete;

	// Add a new state to the machine
	MachineState &addState(const std::string &stateName)
	{
		auto state = std::make_unique<MachineState>(stateName);
		auto &stateRef = *state;
		states[stateName] = std::move(state);

		// Set as current if it's the first state
		if (currentState.empty()) {
			currentState = stateName;
		}

		return stateRef;
	}

	// Check if a state exists
	bool hasState(const std::string &stateName) const { return states.find(stateName) != states.end(); }

	// Get current state
	MachineState *getCurrentState()
	{
		auto it = states.find(currentState);
		return it != states.end() ? it->second.get() : nullptr;
	}

	// Transition to a new state
	void transitionTo(const std::string &newState, Registry &registry, Entity entity)
	{
		if (!hasState(newState) || currentState == newState) {
			return;
		}

		// Exit current state
		if (auto *current = getCurrentState()) {
			for (auto &action : current->onExitActions) {
				action->execute(registry, entity, 0.0f);
			}
		}

		previousState = currentState;
		currentState = newState;
		timeInState = 0.0f;
		justEnteredState = true;

		// Enter new state
		if (auto *newStatePtr = getCurrentState()) {
			for (auto &action : newStatePtr->onEnterActions) {
				action->execute(registry, entity, 0.0f);
			}
		}
	}
};

} // namespace GameEngine::ECS
