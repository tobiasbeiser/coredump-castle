#pragma once
#include <glm/vec2.hpp>

struct TileComponent {
	glm::vec2 TileSize = {32,32};
	glm::vec2 TileMapPosition = {0, 0};
};