#pragma once
#include <glm/glm.hpp>

struct PatrolComponent {
	glm::vec2 minBounds;
	glm::vec2 maxBounds;
	float speed = 50.0f;
	int direction = 1;
};