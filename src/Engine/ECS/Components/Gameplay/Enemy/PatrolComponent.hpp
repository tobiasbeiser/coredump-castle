#pragma once
#include <glm/glm.hpp>

struct PatrolComponent {
	glm::vec2 minBounds{};
	glm::vec2 maxBounds{};
};