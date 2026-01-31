#pragma once

#include "Engine/ECS/Entity.hpp"

#include <SDL3/SDL_rect.h>
#include <glm/vec2.hpp>

struct HitBoxComponent;
namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::Utils::HitBoxUtils {

SDL_FRect getWeaponHitBoxRect(const glm::vec2 &pos, const SDL_FRect &weaponHitBox, const SDL_FRect &bodyHitBox,
                              const glm::vec2 *scale, const bool facingLeft);

SDL_FRect getHitBoxFacing(const glm::vec2 &pos, const SDL_FRect &hitbox, const SDL_FRect &referenceBox,
                          const glm::vec2 *scale, const bool facingLeft);

SDL_FRect getHitBox(const glm::vec2 &pos, const SDL_FRect &hitbox, const glm::vec2 *scale);
} // namespace GameEngine::Utils::HitBoxUtils