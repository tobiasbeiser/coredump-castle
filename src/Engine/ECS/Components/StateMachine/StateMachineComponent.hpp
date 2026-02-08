#pragma once

#include "Engine/ECS/Entity.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Game/AI/States.hpp"
#include <functional>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace GameEngine::ECS {

class Registry;

struct StateData {
	virtual ~StateData() = default;
};
struct Transition {
	AI::AIState targetState;

	virtual ~Transition() = default;
	virtual bool canTransition(Registry &registry, Entity entity, float deltaTime) const = 0;
};

struct StateAction {
	virtual ~StateAction() = default;
	virtual void execute(Registry &registry, Entity entity, float deltaTime) = 0;
};

struct CompositeAction : StateAction {
	std::vector<std::unique_ptr<StateAction>> actions;

	void execute(Registry &registry, Entity entity, float deltaTime) override
	{
		for (const auto &action : actions) {
			action->execute(registry, entity, deltaTime);
		}
	}

  protected:
	template <typename T, typename... Args>
	void add(Args &&...args)
	{
		actions.push_back(std::make_unique<T>(std::forward<Args>(args)...));
	}
};

struct AndTransition : Transition {
	std::vector<std::unique_ptr<Transition>> conditions;

	explicit AndTransition(const AI::AIState target) { this->targetState = target; }

	bool canTransition(Registry &registry, const Entity entity, const float deltaTime) const override
	{
		for (const auto &condition : conditions) {
			if (!condition->canTransition(registry, entity, deltaTime)) {
				return false;
			}
		}
		return true;
	}

	template <typename T, typename... Args>
	AndTransition &add(Args &&...args)
	{
		conditions.push_back(std::make_unique<T>(AI::AIState::None, std::forward<Args>(args)...));
		return *this;
	}
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
	std::unordered_map<AI::AIState, std::unique_ptr<MachineState>> states;
	AI::AIState currentState = AI::AIState::None;
	AI::AIState previousState = AI::AIState::None;

	float timeInState = 0.0f;
	bool justEnteredState = false;

	[[nodiscard]] std::string getDebugInfo() const
	{
		std::ostringstream oss;
		oss << "Current: " << AI::toString(currentState) << "\n"
		    << "Time: " << std::fixed << std::setprecision(1) << timeInState << "s\n"
		    << "Prev: " << AI::toString(previousState);
		return oss.str();
	}

	std::vector<std::unique_ptr<StateAction>> globalUpdateActions;

	StateMachineComponent() = default;
	StateMachineComponent(StateMachineComponent &&) = default;
	StateMachineComponent &operator=(StateMachineComponent &&) = default;
	StateMachineComponent(const StateMachineComponent &) = delete;
	StateMachineComponent &operator=(const StateMachineComponent &) = delete;

	MachineState &addState(const AI::AIState state)
	{
		auto machineState = std::make_unique<MachineState>(std::string(AI::toString(state)));
		states[state] = std::move(machineState);

		if (currentState == AI::AIState::None) {
			currentState = state;
		}

		return *states[state];
	}

	bool hasState(const AI::AIState state) const { return states.contains(state); }

	MachineState *getCurrentState()
	{
		const auto it = states.find(currentState);
		return it != states.end() ? it->second.get() : nullptr;
	}

	void transitionTo(const AI::AIState newState, Registry &registry, const Entity entity)
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

		if (const auto *newStatePtr = getCurrentState()) {
			for (auto &action : newStatePtr->onEnterActions) {
				action->execute(registry, entity, 0.0f);
			}
		}
	}
};

} // namespace GameEngine::ECS
