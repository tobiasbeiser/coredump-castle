#pragma once
#include "Engine/ECS/Entity.hpp"

#include <SDL3/SDL_pixels.h>

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::Utils::Blink {

SDL_Color getBlinkColor(GameEngine::ECS::Registry &registry, GameEngine::ECS::Entity entity);

} // namespace GameEngine::Utils::Blink
