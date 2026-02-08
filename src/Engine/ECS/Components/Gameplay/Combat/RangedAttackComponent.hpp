#pragma once
#include <glm/vec2.hpp>

struct RangedAttackComponent {
	float projectileSpeed = 1.0f;
	glm::vec2 projectileOffset = {0.0f, 0.0f};
	bool projectileSpawned = false;
	glm::vec2 targetSnapshot = glm::vec2(0.0f);
	float attackRange = 300.0f;

	// Spread shot configuration
	bool useSpreadShot = false;
	int spreadCount = 3;          // Number of arrows in spread (must be odd for center shot)
	float spreadAngle = 15.0f;    // Angle between each arrow (degrees)
};