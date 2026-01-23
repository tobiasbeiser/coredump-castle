#pragma once

#include "Engine/ECS/Entity.hpp"

#include <SDL3/SDL_rect.h>

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::Utils::HitBoxUtils {

SDL_FRect getWeaponHitBoxRect(ECS::Registry &registry, ECS::Entity entity);

SDL_FRect getHurtBoxRect(ECS::Registry &registry, ECS::Entity entity);
} // namespace GameEngine::Utils::HitBoxUtils