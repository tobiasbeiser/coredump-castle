#pragma once
#include "Engine/ECS/Components/StateMachine/StateMachineComponent.hpp"

namespace GameEngine::ECS {

class StateMachineBuilder {
  public:
	explicit StateMachineBuilder(StateMachineComponent &machine)
	    : stateMachine(machine), currentStateName(AI::AIState::None)
	{
	}

	StateMachineBuilder &addState(const AI::AIState stateName)
	{
		currentStateName = stateName;
		stateMachine.addState(stateName);
		return *this;
	}

	template <typename ActionType, typename... Args>
	StateMachineBuilder &addGlobalUpdate(Args &&...args)
	{
		stateMachine.globalUpdateActions.push_back(std::make_unique<ActionType>(std::forward<Args>(args)...));
		return *this;
	}

	template <typename ActionType, typename... Args>
	StateMachineBuilder &onEnter(Args &&...args)
	{
		if (currentStateName == AI::AIState::None)
			return *this;

		if (auto *state = stateMachine.states[currentStateName].get()) {
			state->onEnterActions.push_back(std::make_unique<ActionType>(std::forward<Args>(args)...));
		}
		return *this;
	}

	template <typename ActionType, typename... Args>
	StateMachineBuilder &onUpdate(Args &&...args)
	{
		if (currentStateName == AI::AIState::None)
			return *this;

		if (auto *state = stateMachine.states[currentStateName].get()) {
			state->onUpdateActions.push_back(std::make_unique<ActionType>(std::forward<Args>(args)...));
		}
		return *this;
	}

	template <typename ActionType, typename... Args>
	StateMachineBuilder &onExit(Args &&...args)
	{
		if (currentStateName == AI::AIState::None)
			return *this;

		if (auto *state = stateMachine.states[currentStateName].get()) {
			state->onExitActions.push_back(std::make_unique<ActionType>(std::forward<Args>(args)...));
		}
		return *this;
	}

	template <typename TransitionType, typename... Args>
	StateMachineBuilder &transition(Args &&...args)
	{
		if (currentStateName == AI::AIState::None)
			return *this;

		if (auto *state = stateMachine.states[currentStateName].get()) {
			state->transitions.push_back(std::make_unique<TransitionType>(std::forward<Args>(args)...));
		}
		return *this;
	}



	StateMachineBuilder &setInitialState(const AI::AIState stateName)
	{
		stateMachine.currentState = stateName;
		stateMachine.previousState = AI::AIState::None;
		stateMachine.timeInState = 0.0f;
		stateMachine.justEnteredState = true;
		return *this;
	}

	StateMachineComponent &build() const { return stateMachine; }

  private:
	StateMachineComponent &stateMachine;
	AI::AIState currentStateName;
};

} // namespace GameEngine::ECS
