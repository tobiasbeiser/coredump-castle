#pragma once
#include "Engine/Utils/Timer.hpp"

using namespace std::chrono_literals;

struct HealthComponent {
	int health{0};
	int maxHealth{0};
	Timer invulnerabilityTimer{std::chrono::duration<float>(500ms)};

	HealthComponent() = default;

	HealthComponent(const int health, const int maxHealth,
	                const std::chrono::duration<float> invulnerabilityDuration = std::chrono::duration<float>(500ms))
	    : health(health), maxHealth(maxHealth), invulnerabilityTimer(invulnerabilityDuration, true)
	{
	}
};