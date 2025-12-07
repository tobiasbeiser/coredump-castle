#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include <SDL3/SDL_rect.h>

namespace GameEngine::Utils {
	[[nodiscard]] inline SDL_FRect getBoundingBox(const glm::vec2 &pos, const SDL_FRect &box) {
			return SDL_FRect{
				pos.x + box.x,
				pos.y + box.y,
				box.w,
				box.h
			};
		}
} // namespace GameEngine::Utils