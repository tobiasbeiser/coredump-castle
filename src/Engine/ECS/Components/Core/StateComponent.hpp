#pragma once
#include <string>

enum class State { Idle, Walking, Jumping, Falling, Attacking };

inline std::string toString(State state)
{
	switch (state) {
	case State::Idle:
		return "Idle";
	case State::Walking:
		return "Walking";
	case State::Jumping:
		return "Jumping";
	case State::Falling:
		return "Falling";
	case State::Attacking:
		return "Attacking";
	default:
		return "Unknown";
	}
}

struct StateComponent {
	State state = State::Idle;
};