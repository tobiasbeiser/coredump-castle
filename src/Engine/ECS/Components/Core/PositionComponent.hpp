#pragma once
#include <glm/glm.hpp>

struct PositionComponent {
	glm::vec2 position{0.0f, 0.0f};
	glm::vec2 previousPosition = position;
};