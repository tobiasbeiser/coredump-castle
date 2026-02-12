#pragma once
#include <SDL3/SDL_rect.h>
#include <glm/vec2.hpp>

namespace Ui {
struct AnchorParams;
class Widget;
struct UIContext;
} // namespace Ui

namespace Ui::LayoutEngine {
static constexpr int framePixelOffset = 4;

enum class Orientation { Vertical, Horizontal };
enum class Alignment { Start, Center, End, Stretch };

// Data required to solve a single item.
// Separates the Widget pointer from the layout math.
struct LayoutItem {
	Widget* widget = nullptr;
	float weight = 0.0f;       // 0.0f = Fixed size, >0.0f = Flex
	Alignment align = Alignment::Center;
	glm::vec2 idealSize = {0.0f, 0.0f}; // Pre-calculated before solving
};

struct LinearConfig {
	Orientation orientation = Orientation::Vertical;
	float spacing = 0.0f;
	glm::vec2 padding = {0.0f, 0.0f};
};

SDL_FRect positionAnchored(glm::vec2 allocatedSize, glm::vec2 ownSize, const AnchorParams &anchorParams);
SDL_FRect positionCentered(glm::vec2 allocatedSize, glm::vec2 ownSize);
SDL_FRect positionFrameChild(glm::vec2 frameSize, int outlineOffset = framePixelOffset);
void layoutAnchored(Widget *widget, const UIContext &ctx, SDL_FRect allocatedRect, AnchorParams params);

} // namespace Ui::LayoutEngine