#pragma once
#include "Engine/ECS/Registry.hpp"

#include <SDL3/SDL_rect.h>


namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::MapTrackingSystem {

	void update(Registry& registry, const SDL_FRect& viewport);

}