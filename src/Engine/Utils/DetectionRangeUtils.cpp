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

} // namespace GameEngine::Utils::DetectionRangeUtils
