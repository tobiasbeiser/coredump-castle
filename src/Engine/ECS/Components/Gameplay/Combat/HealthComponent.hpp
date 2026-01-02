#pragma once
#include "Engine/Utils/Timer.hpp"

struct HealthComponent {
	int health{0};
	int maxHealth{0};
	Timer invulnerabilityTimer{std::chrono::duration<float>(1.0f)};

	HealthComponent() = default;

	HealthComponent(const int health, const int maxHealth,
	                const std::chrono::duration<float> invulnerabilityDuration = std::chrono::duration<float>(1.0f))
	    : health(health), maxHealth(maxHealth), invulnerabilityTimer(invulnerabilityDuration, true)
	{
	}
};