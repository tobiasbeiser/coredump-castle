#pragma once

struct RangedAttackComponent {
	float detectionRange = 0.0f;
	float projectileSpeed = 1.0f;
	glm::vec2 projectileOffset = {0.0f, 0.0f};
	bool projectileSpawned = false;
};