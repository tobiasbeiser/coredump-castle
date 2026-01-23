#include "DetectionRangeUtils.hpp"

#include <cmath>

namespace GameEngine::Utils::DetectionRangeUtils {

glm::vec2 calculateCenter(const glm::vec2 &position, const SDL_FRect &box, const glm::vec2 &scale)
{
	return {position.x + (box.x * scale.x) + (box.w * scale.x) * 0.5f,
	        position.y + (box.y * scale.y) + (box.h * scale.y) * 0.5f};
}

glm::vec2 getRectCenter(const SDL_FRect &rect)
{
	return {rect.x + (rect.w * 0.5f), rect.y + (rect.h * 0.5f)};
}

bool isWithinDetectionRange(const glm::vec2 &enemyCenter, const glm::vec2 &playerCenter, const ChaseComponent &chase)
{
	const float xDistance = std::abs(playerCenter.x - enemyCenter.x);
	const float yDistance = std::abs(playerCenter.y - enemyCenter.y);

	return (!chase.detectionWidthRange.has_value() || xDistance <= chase.detectionWidthRange.value())
	       && yDistance <= chase.detectionHeightRange;
}

float getVerticalThreshold(const ChaseComponent &chase, const bool playerGrounded)
{
	return playerGrounded ? chase.detectionHeightRange : chase.detectionHeightRange * 2.0f;
}

SDL_FRect getMeleeDetectionRect(const PatrolComponent &patrol, const ChaseComponent &chase)
{
	SDL_FRect rangeRect;
	rangeRect.w = patrol.maxBounds.x - patrol.minBounds.x;
	rangeRect.h = chase.detectionHeightRange * 2.0f;
	rangeRect.x = patrol.minBounds.x;
	rangeRect.y = patrol.minBounds.y - chase.detectionHeightRange;
	return rangeRect;
}

SDL_FRect getRangedDetectionRect(const ChaseComponent &chase, const glm::vec2 &center)
{
	SDL_FRect rangeRect;
	const auto detectionWidth = chase.detectionWidthRange.value_or(0.0f);
	rangeRect.w = detectionWidth * 2.0f;
	rangeRect.h = chase.detectionHeightRange * 2.0f;
	rangeRect.x = center.x - detectionWidth;
	rangeRect.y = center.y - chase.detectionHeightRange;
	return rangeRect;
}

} // namespace GameEngine::Utils::DetectionRangeUtils
