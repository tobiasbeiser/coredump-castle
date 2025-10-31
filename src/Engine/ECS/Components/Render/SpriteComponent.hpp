#pragma once
#include <glm/vec2.hpp>
#include <SDL3/SDL_render.h>

struct SpriteComponent {
	SDL_Texture* texture;
	glm::ivec2 TileSize = {32,32};
	glm::ivec2 TileMapPosition = {0, 0};
};