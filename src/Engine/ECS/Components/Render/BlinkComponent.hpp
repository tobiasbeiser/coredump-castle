#pragma once
#include <SDL3/SDL_pixels.h>

struct BlinkComponent {
	SDL_Color color{165, 0, 0, 155};
	float blinkFrequency{0.2f};
};