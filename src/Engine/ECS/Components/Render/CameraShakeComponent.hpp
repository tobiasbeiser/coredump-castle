#pragma once

#include "Engine/Utils/Timer.hpp"

#include <chrono>
#include <glm/vec2.hpp>

struct CameraShakeComponent {
	Timer duration{std::chrono::milliseconds(200)};
	float amplitude = 6.0f;
	bool decay = true;
	glm::vec2 offset{0.0f, 0.0f};
};
