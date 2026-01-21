#pragma once

#include <glm/vec2.hpp>
#include <string>

struct ProjectileComponent {
	float maxRange = 0.0f;
	float speed = 1.0f;
	glm::vec2 spawnPosition = {0.0f, 0.0f};
	std::string projectileTilesetPath = "";
	glm::vec2 projectileSize = {1.0f, 1.0f};
	glm::vec2 spriteSize = {32.0f, 32.0f};
	glm::vec2 hitboxOffset = {0.0f, 0.0f};
};