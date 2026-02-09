#pragma once
#include <string>

enum class State { Idle, Walking, Rolling, Jumping, Falling, Attacking, Hurting, Interacting, Dead };

inline std::string toString(const State state)
{
	switch (state) {
	case State::Idle:
		return "Idle";
	case State::Walking:
		return "Walking";
	case State::Rolling:
		return "Rolling";
	case State::Jumping:
		return "Jumping";
	case State::Falling:
		return "Falling";
	case State::Attacking:
		return "Attacking";
	case State::Hurting:
		return "Hurting";
	case State::Interacting:
		return "Interacting";
	case State::Dead:
		return "Dead";
	default:
		return "Unknown";
	}
}

struct StateComponent {
	State state = State::Idle;
};