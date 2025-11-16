#pragma once
#include <glm/glm.hpp>

struct PatrolComponent {
	glm::vec2 minBounds; // Left boundary
	glm::vec2 maxBounds; // Right boundary
	float speed = 50.0f;
	int direction = 1; // 1 = right, -1 = left
};