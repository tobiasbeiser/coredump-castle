#pragma once
#include <SDL3/SDL_pixels.h>

struct OutlineGlowComponent {
	SDL_Color outlineColor{255, 200, 0, 255};
	float thickness{2.0f};
	float intensity{1.0f};
	bool active{false};
};
