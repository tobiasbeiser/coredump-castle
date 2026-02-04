#pragma once
#include "Engine/Utils/Timer.hpp"
#include <optional>

struct ChaseComponent {
	float chaseSpeed = 150.0f;
	float detectionRange = 200.0f;
	float giveUpRange = 350.0f;
	Timer reactionDelay{std::chrono::duration<float>::zero()};
	Timer chargeCooldown{std::chrono::duration<float>(3.0f)};
};
