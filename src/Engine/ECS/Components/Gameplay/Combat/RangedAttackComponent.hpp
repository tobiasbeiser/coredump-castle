#pragma once
#include "Engine/Utils/Timer.hpp"
#include <chrono>
#include <glm/vec2.hpp>

struct RangedAttackComponent {
	float projectileSpeed = 1.0f;
	glm::vec2 projectileOffset = {0.0f, 0.0f};
	bool projectileSpawned = false;
	glm::vec2 targetSnapshot = glm::vec2(0.0f);
	float attackRange = 300.0f;

	bool useSpreadShot = false;
	int spreadCount = 0;
	float spreadAngle = 0.0f;
	float spreadMinRange = 0.0f;
	Timer spreadCooldownTimer{std::chrono::duration<float>(0.0f), true};
	Timer spreadTelegraphTimer{std::chrono::duration<float>(0.0f), true};
};