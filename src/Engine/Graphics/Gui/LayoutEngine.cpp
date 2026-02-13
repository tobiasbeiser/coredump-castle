#include "LayoutEngine.hpp"

#include "Base/Widget.hpp"
#include "Layouts/AnchorLayout.hpp"

namespace Ui::LayoutEngine {

SDL_FRect positionAnchored(const glm::vec2 allocatedSize, const glm::vec2 ownSize, const AnchorParams &anchorParams)
{
	const auto [anchor, pivot, offset] = anchorParams;

	const glm::vec2 anchorPos = allocatedSize * anchor;
	const glm::vec2 pivotPos = ownSize * pivot;
	const glm::vec2 finalPos = anchorPos - pivotPos + offset;
	return {std::clamp(finalPos.x, 0.f, allocatedSize.x), std::clamp(finalPos.y, 0.f, allocatedSize.y), ownSize.x,
	        ownSize.y};
}

SDL_FRect positionCentered(const glm::vec2 allocatedSize, const glm::vec2 ownSize)
{
	return positionAnchored(allocatedSize, ownSize, AnchorParams::center);
}

SDL_FRect positionFrameChild(const glm::vec2 frameSize, const int outlineOffset)
{
	const glm::vec2 childSize = {frameSize.x - outlineOffset, frameSize.y - outlineOffset};
	return positionAnchored(frameSize, childSize, AnchorParams::center);
}

SDL_FRect positionFrame(const SDL_FRect contentBounds, const int outlineOffset)
{
	const glm::vec2 frameSize = {contentBounds.x - outlineOffset, contentBounds.y - outlineOffset};
	return {frameSize.x, frameSize.y, contentBounds.w + 2 * outlineOffset, contentBounds.h + 2 * outlineOffset};
}

void layoutAnchored(Widget *widget, const UIContext &ctx, const SDL_FRect allocatedRect, const AnchorParams &params)
{
	const glm::vec2 parentSize = {allocatedRect.w, allocatedRect.h};

	const BoxConstraints measureBox = BoxConstraints::loose(parentSize);
	const glm::vec2 widgetSize = widget->measure(ctx, measureBox);

	widget->layout(ctx, positionAnchored(parentSize, widgetSize, params));
}

} // namespace Ui::LayoutEngine