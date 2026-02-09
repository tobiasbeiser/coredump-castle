#pragma once

#include "Engine/Utils/Timer.hpp"

#include <chrono>

struct MovementSlowComponent {
	Timer slowTimer{std::chrono::duration<float>(0.5f)};
	float speedMultiplier = 0.6f;
	float originalMaxSpeedX = 0.0f;
	bool initialized = false;
};
