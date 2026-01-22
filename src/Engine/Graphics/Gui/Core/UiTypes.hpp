#pragma once
#include <algorithm>
#include <limits>
#include <glm/vec2.hpp>
#include <SDL3/SDL_pixels.h>

#include "Engine/Graphics/FontManager.hpp"


namespace Ui
{

	constexpr float kInfinity = std::numeric_limits<float>::infinity();

	constexpr bool operator==(const SDL_Color &a, const SDL_Color &b) noexcept
	{
		return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
	}

	// Defines the constraints passed from Parent to Child during layout.
	struct BoxConstraints {
		float minWidth = 0.0f;
		float maxWidth = kInfinity;
		float minHeight = 0.0f;
		float maxHeight = kInfinity;

		// Helper: Create tight constraints (force specific size)
		static BoxConstraints tight(const float width, const float height) { return {width, width, height, height}; }

		// Helper: Loosen constraints (0 to max)
		static BoxConstraints loose(const glm::vec2 size) { return {0.0f, size.x, 0.0f, size.y}; }

		[[nodiscard]] glm::vec2 constrain(const glm::vec2 size) const
		{
			return {std::clamp(size.x, minWidth, maxWidth), std::clamp(size.y, minHeight, maxHeight)};
		}

		[[nodiscard]] bool hasBoundedWith() const { return maxWidth < kInfinity; }
		[[nodiscard]] bool hasBoundedHeight() const { return maxHeight < kInfinity; }
	};

	// Controls how a child aligns within the parent's cross-axis
	enum class CrossAxisAlignment {
		Start,
		Center,
		End,
		Stretch // Force child to fill cross-axis
	};
	enum class WidgetState : uint8_t { Normal, Hovered, Pressed, Disabled };

	enum class SizeMode {
		Fixed,       // Use explicit width/height
		WrapContent, // Shrink to fit content (e.g., text size)
		MatchParent  // Expand to fill available parent space
	};

	enum class Orientation {
		Vertical,
		Horizontal,
	};

	struct InternalDimensions {
		float totalFlex = 0.0f;
		float fixedSpace = 0.0f;
		float maxCrossAxis = 0.0f;
		float remainingSpace = 0.0f;
	};

}
