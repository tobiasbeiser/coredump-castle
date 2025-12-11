#pragma once
#include <glm/vec2.hpp>

struct TransformComponent {
	glm::vec2 scale = glm::vec2(1.0f, 1.0f);
	float rotation = 0.0f; // Rotation in radians
};
