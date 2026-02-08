#pragma once
#include "Engine/Utils/Timer.hpp"
#include <SDL3/SDL_pixels.h>
#include <chrono>

struct ChargeTelegraphComponent {
	SDL_Color glowColor{255, 140, 0, 255};
	float duration{1.0f};
	float pulseFrequency{0.15f};
	float outlineThickness{4.0f};
	bool useOutlineGlow{true};
	Timer telegraphTimer{std::chrono::duration<float>(1.0f)};
	bool active{false};

	void start()
	{
		telegraphTimer = Timer(std::chrono::duration<float>(duration));
		active = true;
	}

	[[nodiscard]] bool isFinished() const { return telegraphTimer.isTimeout(); }
};
