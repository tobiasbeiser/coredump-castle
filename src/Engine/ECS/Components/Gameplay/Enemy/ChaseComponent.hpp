#pragma once
#include "Engine/Utils/Timer.hpp"
#include <optional>

struct ChaseComponent {
	float chaseSpeed = 0.0f;
	float originalPatrolSpeed = 0.0f;
	Timer reactionDelay{std::chrono::duration<float>::zero()};
	float detectionHeightRange = 50.0f;
	std::optional<float> detectionWidthRange = std::nullopt;
	std::optional<float> lastKnownPlayerX = std::nullopt;
};
