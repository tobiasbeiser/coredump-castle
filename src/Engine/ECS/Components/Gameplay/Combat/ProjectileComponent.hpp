#pragma once

#include <glm/vec2.hpp>
#include <string>

struct ProjectileComponent {
	float maxRange = 0.0f;
	float speed = 1.0f;
	glm::vec2 spawnPosition = {0.0f, 0.0f};
	std::string projectileTilesetPath = "";
};