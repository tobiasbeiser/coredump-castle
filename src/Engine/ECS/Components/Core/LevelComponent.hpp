#pragma once
#include <glm/vec2.hpp>
#include <string>

struct LevelComponent {
	glm::vec2 levelSize{0.0f, 0.0f};
	std::string levelName{""};
};