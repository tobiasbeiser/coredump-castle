#pragma once

#include "StateMachineComponent.hpp"
#include "StateTransitionsAndActions.hpp"
#include <memory>

namespace GameEngine::ECS {

/**
 * Builder for constructing state machines fluently
 * Example usage:
 *
 * auto& sm = registry.registerComponent<StateMachineComponent>(entity, StateMachineComponent{});
 *
 * StateMachineBuilder(sm)
 *     .addState("idle")
 *         .onEnter<SetVelocityAction>(glm::vec2{0, 0})
 *         .onUpdate<FacePlayerAction>()
 *         .transition<ProximityEntryTransition>("telegraphing", 150.0f)
 *     .addState("telegraphing")
 *         .onEnter<TriggerAnimationAction>("windup", false)
 *         .onUpdate<FacePlayerAction>()
 *         .transition<AnimationTransition>("attacking", "windup")
 *     .addState("attacking")
 *         .onEnter<TriggerAnimationAction>("attack", false)
 *         .onEnter<ExecuteAttackAction>()
 *         .transition<AnimationTransition>("recover", "attack")
 *     .addState("recover")
 *         .onEnter<SetVelocityAction>(glm::vec2{0, 0})
 *         .transition<TimerTransition>("idle", 1.0f)
 *     .build();
 */
class StateMachineBuilder {
  public:
	explicit StateMachineBuilder(StateMachineComponent &machine) : stateMachine(machine), currentStateName("") {}

	// Add a new state and make it the current context for building
	StateMachineBuilder &addState(const std::string &stateName)
	{
		currentStateName = stateName;
		stateMachine.addState(stateName);
		return *this;
	}

	// Add an action to execute when entering the current state
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

	// Add an action to execute while in the current state (every frame)
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

	// Add an action to execute when exiting the current state
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

	// Add a transition from the current state
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

	// Set custom state data
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

	// Set the initial state
	StateMachineBuilder &setInitialState(const std::string &stateName)
	{
		stateMachine.currentState = stateName;
		stateMachine.previousState = "";
		stateMachine.timeInState = 0.0f;
		stateMachine.justEnteredState = true;
		return *this;
	}

	// Finish building (returns reference for convenience)
	StateMachineComponent &build() { return stateMachine; }

  private:
	StateMachineComponent &stateMachine;
	std::string currentStateName;
};

} // namespace GameEngine::ECS
