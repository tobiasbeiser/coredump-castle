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

SDL_FRect positionAnchored(glm::vec2 allocatedSize, glm::vec2 ownSize, const AnchorParams &anchorParams);
SDL_FRect positionCentered(glm::vec2 allocatedSize, glm::vec2 ownSize);
SDL_FRect positionFrameChild(glm::vec2 frameSize, int outlineOffset = framePixelOffset);
void layoutAnchored(Widget *widget, const UIContext &ctx, SDL_FRect allocatedRect, AnchorParams params);

} // namespace Ui::LayoutEngine