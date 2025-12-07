#pragma once
#include "Engine/Utils/Timer.hpp"
#include <SDL3/SDL_pixels.h>

struct BlinkComponent {
	Timer timer{0.0f};
	SDL_Color color{255, 0, 0, 155};
	float blinkFrequency{0.1f};

	explicit BlinkComponent(const float duration = 1.0f) : timer(duration)
	{
		timer.forceTimeout();
	}

};