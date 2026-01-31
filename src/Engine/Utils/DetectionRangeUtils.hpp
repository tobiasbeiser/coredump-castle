#pragma once

#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"

#include <SDL3/SDL_rect.h>
#include <glm/vec2.hpp>
#include <optional>

namespace GameEngine::Utils::DetectionRangeUtils {

glm::vec2 calculateCenter(const glm::vec2 &position, const SDL_FRect &box, const glm::vec2 &scale);

glm::vec2 getRectCenter(const SDL_FRect &rect);

bool isWithinDetectionRange(const glm::vec2 &enemyCenter, const glm::vec2 &playerCenter, const ChaseComponent &chase);

float getVerticalThreshold(const ChaseComponent &chase, bool playerGrounded);

SDL_FRect getMeleeDetectionRect(const PatrolComponent &patrol, const ChaseComponent &chase);

SDL_FRect getRangedDetectionRect(const ChaseComponent &chase, const glm::vec2 &center);

bool checkLineOfSight(const glm::vec2 &start, const glm::vec2 &targetCenter, const SDL_FRect &targetBox, const glm::vec2 &targetScale, const glm::vec2 &projectileSize);

// Returns the target position that has LOS - either center or head
std::optional<glm::vec2> getTargetWithLineOfSight(const glm::vec2 &start, const glm::vec2 &targetCenter, const SDL_FRect &targetBox, const glm::vec2 &targetScale, const glm::vec2 &projectileSize);

} // namespace GameEngine::Utils::DetectionRangeUtils
