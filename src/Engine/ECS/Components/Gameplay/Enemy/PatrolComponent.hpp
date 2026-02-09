#pragma once
#include <glm/glm.hpp>
#include <optional>

struct PatrolComponent {
	float patrolSpeed{20.0f};
	glm::vec2 patrolCenter{0.0f, 0.0f};
	std::optional<float> minX{std::nullopt};
	std::optional<float> maxX{std::nullopt};
};