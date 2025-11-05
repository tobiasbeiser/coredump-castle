#pragma once
#include <glm/vec2.hpp>
#include <SDL3/SDL_render.h>

struct SpriteComponent {
	SDL_Texture* texture{};
	glm::ivec2 tileSize = {32,32};
	glm::ivec2 tileMapPosition = {0, 0};
};