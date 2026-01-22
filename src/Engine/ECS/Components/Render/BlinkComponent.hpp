#pragma once
#include <SDL3/SDL_pixels.h>

struct BlinkComponent {
	SDL_Color color{165, 0, 0, 200};
	float blinkFrequency{0.08f};
};