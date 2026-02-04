#pragma once

#include "Engine/Utils/Timer.hpp"

#include <chrono>

struct ChargeComponent {
	Timer chargeTimer{std::chrono::duration<float>(1.5f)};
	Timer chargePauseTimer{std::chrono::duration<float>(0.5f)};
	float chargeSpeedMultiplier = 2.0f;
	float chargeDirection = 0.0f;
};