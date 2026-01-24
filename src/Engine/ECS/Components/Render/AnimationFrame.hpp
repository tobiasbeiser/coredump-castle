#pragma once
#include <SDL3/SDL_render.h>
#include <glm/vec2.hpp>

struct AnimationFrame {
	SDL_Texture *texture{};
	glm::ivec2 tileSize{};
	glm::ivec2 tileMapPosition{};
	bool damageActive = false;
};
