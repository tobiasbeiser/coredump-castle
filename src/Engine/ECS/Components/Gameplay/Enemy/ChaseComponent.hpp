#pragma once
#include "Engine/Utils/Timer.hpp"

struct ChaseComponent {
	float verticalDetectionHeight = 50.0f;
	float chaseSpeed = 0.0f;
	float originalPatrolSpeed = 0.0f;
	Timer reactionDelay{std::chrono::duration<float>::zero()};
};
