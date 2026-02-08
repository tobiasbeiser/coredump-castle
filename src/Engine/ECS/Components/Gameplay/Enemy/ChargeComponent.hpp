#pragma once

#include "Engine/Utils/Timer.hpp"

#include <chrono>

struct ChargeComponent {
	Timer chargeCooldown{std::chrono::duration<float>(0.0f), true};
	Timer chargeTimer{std::chrono::duration<float>(0.0f)};
	Timer chargePauseTimer{std::chrono::duration<float>(0.0f)};
	float chargeSpeedMultiplier = 0.0f;
	float chargeChance = 0.0f;
	float chargeDirection = 0.0f;
};