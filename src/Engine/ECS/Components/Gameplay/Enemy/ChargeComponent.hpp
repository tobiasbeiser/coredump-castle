#pragma once

#include "Engine/Utils/Timer.hpp"

#include <chrono>

struct ChargeComponent {
	Timer chargeTimer{std::chrono::duration<float>(0.8f)};
	Timer chargePauseTimer{std::chrono::duration<float>(0.1f)};
	Timer chargeCooldown{std::chrono::duration<float>(3.0f)};
	float chargeChance = 0.35f;
	float chargeSpeedMultiplier = 2.0f;
	float chargePassThroughFactor = 0.35f;
};