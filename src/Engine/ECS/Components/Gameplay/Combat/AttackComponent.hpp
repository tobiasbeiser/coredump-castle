#pragma once
#include "Engine/Utils/Timer.hpp"

struct AttackComponent {
	Timer attackTimer{std::chrono::duration<float>::zero()};
	Timer attackCoolDownTimer{std::chrono::duration<float>::zero()};
	float windUpLimit = 0.0f;       // Configured max limit
	float activeWindUpLimit = 0.0f; // Randomized limit for current attack
};
