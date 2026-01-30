#pragma once
#include <SDL3/SDL_rect.h>
#include <compare>
#include <glm/vec2.hpp>

namespace GameEngine::ECS {
class Registry;
}
namespace GameEngine::Utils::ReferenceFrameUtils {
struct ReferenceFrame {
	int x, y, w, h;
	int playerLayer;
	auto operator<=>(const ReferenceFrame &) const = default;
};

bool pointInRect(glm::vec2 point, SDL_FRect rect);
bool isInFrame(glm::vec2 point, const ReferenceFrame &frame);

ReferenceFrame getReferenceFrame(ECS::Registry &registry);
}; // namespace GameEngine::Utils::ReferenceFrameUtils