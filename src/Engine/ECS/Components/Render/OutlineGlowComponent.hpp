#pragma once
#include <SDL3/SDL_pixels.h>

struct OutlineGlowComponent {
	SDL_Color outlineColor{255, 200, 0, 255}; // Default golden/yellow outline
	float thickness{2.0f};                     // Outline thickness in pixels
	float intensity{1.0f};                     // Current intensity (0.0 to 1.0)
	bool active{false};                        // Whether the glow is currently active
};
