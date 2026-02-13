#pragma once
#include <SDL3/SDL_pixels.h>
#include <algorithm>
#include <glm/vec2.hpp>
#include <limits>

#include "Engine/Graphics/FontManager.hpp"

namespace Ui {

constexpr float kInfinity = std::numeric_limits<float>::infinity();

constexpr bool operator==(const SDL_Color &a, const SDL_Color &b) noexcept
{
	return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

enum class WidgetState : uint8_t { Normal, Hovered, Pressed, Disabled };

enum class SizeMode {
	Fixed,       // Use explicit width/height
	WrapContent, // Shrink to fit content (e.g., text size)
	MatchParent  // Expand to fill available parent space
};

} // namespace Ui
