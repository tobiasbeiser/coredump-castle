#include "DetectionRangeUtils.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"

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
bool checkLineOfSight(const glm::vec2 &start, const glm::vec2 &targetCenter, const SDL_FRect &targetBox,
                      const glm::vec2 &targetScale, const glm::vec2 &projectileSize)
{
	// Target the center
	if (!ECS::Systems::CollisionSystem::isPathObstructed(start, targetCenter, projectileSize)) {
		return true;
	}

	// Target the top (head) - using 25% from top of hurtbox
	const glm::vec2 targetTop = targetCenter - glm::vec2(0.0f, targetBox.h * targetScale.y * 0.25f);
	if (!ECS::Systems::CollisionSystem::isPathObstructed(start, targetTop, projectileSize)) {
		return true;
	}

	return false;
}

std::optional<glm::vec2> getTargetWithLineOfSight(const glm::vec2 &start, const glm::vec2 &targetCenter,
                                                  const SDL_FRect &targetBox, const glm::vec2 &targetScale,
                                                  const glm::vec2 &projectileSize)
{
	// Try center first
	if (!ECS::Systems::CollisionSystem::isPathObstructed(start, targetCenter, projectileSize)) {
		return targetCenter;
	}

	// Try head shot
	const glm::vec2 targetTop = targetCenter - glm::vec2(0.0f, targetBox.h * targetScale.y * 0.25f);
	if (!ECS::Systems::CollisionSystem::isPathObstructed(start, targetTop, projectileSize)) {
		return targetTop;
	}

	return std::nullopt;
}
} // namespace GameEngine::Utils::DetectionRangeUtils
