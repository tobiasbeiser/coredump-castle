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

class Registry;

struct StateData {
	virtual ~StateData() = default;
};
struct Transition {
	std::string targetState;

	virtual ~Transition() = default;
	virtual bool canTransition(Registry &registry, Entity entity, float deltaTime) const = 0;
};

struct StateAction {
	virtual ~StateAction() = default;
	virtual void execute(Registry &registry, Entity entity, float deltaTime) = 0;
};
struct MachineState {
	std::string name;

	std::vector<std::unique_ptr<StateAction>> onEnterActions;

	std::vector<std::unique_ptr<StateAction>> onUpdateActions;

	std::vector<std::unique_ptr<StateAction>> onExitActions;

	std::vector<std::unique_ptr<Transition>> transitions;
	std::unique_ptr<StateData> data;

	explicit MachineState(std::string stateName) : name(std::move(stateName)) {}
};

struct StateMachineComponent {
	std::unordered_map<std::string, std::unique_ptr<MachineState>> states;
	std::string currentState;

	std::string previousState;

	float timeInState = 0.0f;

	bool justEnteredState = false;

	StateMachineComponent() = default;
	StateMachineComponent(StateMachineComponent &&) = default;
	StateMachineComponent &operator=(StateMachineComponent &&) = default;
	StateMachineComponent(const StateMachineComponent &) = delete;
	StateMachineComponent &operator=(const StateMachineComponent &) = delete;

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

	bool hasState(const std::string &stateName) const { return states.find(stateName) != states.end(); }

	MachineState *getCurrentState()
	{
		auto it = states.find(currentState);
		return it != states.end() ? it->second.get() : nullptr;
	}

	void transitionTo(const std::string &newState, Registry &registry, Entity entity)
	{
		if (!hasState(newState) || currentState == newState) {
			return;
		}

		if (auto *current = getCurrentState()) {
			for (auto &action : current->onExitActions) {
				action->execute(registry, entity, 0.0f);
			}
		}

		previousState = currentState;
		currentState = newState;
		timeInState = 0.0f;
		justEnteredState = true;

		if (auto *newStatePtr = getCurrentState()) {
			for (auto &action : newStatePtr->onEnterActions) {
				action->execute(registry, entity, 0.0f);
			}
		}
	}
};

} // namespace GameEngine::ECS
