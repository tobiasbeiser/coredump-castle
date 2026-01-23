#pragma once

#include "Engine/ECS/Entity.hpp"

#include <SDL3/SDL_rect.h>
#include <glm/vec2.hpp>

struct HitBoxComponent;
namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::Utils::HitBoxUtils {

SDL_FRect getWeaponHitBoxRect(const glm::vec2 &pos, const HitBoxComponent &hitBox, const SDL_FRect &hurtBox,
                              const glm::vec2 scale, const bool facingLeft = false);

SDL_FRect getHitBox(const glm::vec2 &pos, const SDL_FRect &hitbox, const glm::vec2 &scale);
} // namespace GameEngine::Utils::HitBoxUtils