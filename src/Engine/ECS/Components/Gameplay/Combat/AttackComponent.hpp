#pragma once
#include "Engine/Utils/Timer.hpp"

struct AttackComponent {
	Timer attackTimer{std::chrono::duration<float>::zero()};
	Timer attackCoolDownTimer{std::chrono::duration<float>::zero()};
	float currentWindUpLimit = 0.0f;
	float minWindUpLimit = 0.0f;
	float maxWindUpLimit = 0.0f;
};
