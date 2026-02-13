#pragma once
#include <SDL3/SDL_rect.h>
#include <string>

struct TooltipComponent {
	std::string text{};
	SDL_FRect box{};
	bool enabled = true;
};