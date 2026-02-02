#pragma once

#include "StateMachineComponent.hpp"
#include "StateTransitionsAndActions.hpp"
#include <memory>

namespace GameEngine::ECS {

class StateMachineBuilder {
  public:
	explicit StateMachineBuilder(StateMachineComponent &machine) : stateMachine(machine), currentStateName("") {}

	StateMachineBuilder &addState(const std::string &stateName)
	{
		currentStateName = stateName;
		stateMachine.addState(stateName);
		return *this;
	}

	template <typename ActionType, typename... Args>
	StateMachineBuilder &onEnter(Args &&...args)
	{
		if (currentStateName.empty())
			return *this;

		auto *state = stateMachine.states[currentStateName].get();
		if (state) {
			state->onEnterActions.push_back(std::make_unique<ActionType>(std::forward<Args>(args)...));
		}
		return *this;
	}

	template <typename ActionType, typename... Args>
	StateMachineBuilder &onUpdate(Args &&...args)
	{
		if (currentStateName.empty())
			return *this;

		auto *state = stateMachine.states[currentStateName].get();
		if (state) {
			state->onUpdateActions.push_back(std::make_unique<ActionType>(std::forward<Args>(args)...));
		}
		return *this;
	}

	template <typename ActionType, typename... Args>
	StateMachineBuilder &onExit(Args &&...args)
	{
		if (currentStateName.empty())
			return *this;

		auto *state = stateMachine.states[currentStateName].get();
		if (state) {
			state->onExitActions.push_back(std::make_unique<ActionType>(std::forward<Args>(args)...));
		}
		return *this;
	}

	template <typename TransitionType, typename... Args>
	StateMachineBuilder &transition(Args &&...args)
	{
		if (currentStateName.empty())
			return *this;

		auto *state = stateMachine.states[currentStateName].get();
		if (state) {
			state->transitions.push_back(std::make_unique<TransitionType>(std::forward<Args>(args)...));
		}
		return *this;
	}

	template <typename DataType, typename... Args>
	StateMachineBuilder &withData(Args &&...args)
	{
		if (currentStateName.empty())
			return *this;

		auto *state = stateMachine.states[currentStateName].get();
		if (state) {
			state->data = std::make_unique<DataType>(std::forward<Args>(args)...);
		}
		return *this;
	}

	StateMachineBuilder &setInitialState(const std::string &stateName)
	{
		stateMachine.currentState = stateName;
		stateMachine.previousState = "";
		stateMachine.timeInState = 0.0f;
		stateMachine.justEnteredState = true;
		return *this;
	}

	StateMachineComponent &build() { return stateMachine; }

  private:
	StateMachineComponent &stateMachine;
	std::string currentStateName;
};

} // namespace GameEngine::ECS
