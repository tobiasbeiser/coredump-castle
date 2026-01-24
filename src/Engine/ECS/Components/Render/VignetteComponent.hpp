#pragma once
#include "Engine/Utils/Timer.hpp"

#include <SDL3/SDL_pixels.h>
#include <algorithm>

struct VignetteComponent {
	float intensity{0.0f};
	Timer duration{std::chrono::milliseconds(500)};
	SDL_FColor color{0.0f, 0.0f, 0.0f, 0.5f};

	[[nodiscard]] float getIntensity() const
	{
		if (duration.isTimeout())
			return 0.0f;

		const float ratio = duration.getTime().count() / duration.getLength().count();
		return std::clamp(1.0f - ratio, 0.0f, 1.0f);
	}
};