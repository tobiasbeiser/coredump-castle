#pragma once
#include <glm/vec2.hpp>

struct LevelComponent {
	glm::vec2 levelSize{0.0f, 0.0f};
	int playerLayer{100};
};