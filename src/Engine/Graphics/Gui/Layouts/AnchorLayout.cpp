
#include "AnchorLayout.hpp"

namespace Ui {
// --- Presets ---
const AnchorParams AnchorParams::center = {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.0f, 0.0f}};
const AnchorParams AnchorParams::topLeft = {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}};
const AnchorParams AnchorParams::topRight = {{1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}};
const AnchorParams AnchorParams::bottomCenter = {{0.5f, 1.0f}, {0.5f, 1.0f}, {0.0f, 0.0f}};

void AnchorLayout::addChild(std::unique_ptr<Widget> widget, const AnchorParams params)
{
	anchorParams.push_back(params);
	addWidget(std::move(widget));
}

glm::vec2 AnchorLayout::performMeasure(const UIContext &ctx, const BoxConstraints constraints) const
{
	return {constraints.maxWidth, constraints.maxHeight};
}

void AnchorLayout::layout(const UIContext &ctx, const SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);

	const auto &children = getChildren();

	if (children.size() != anchorParams.size()) {
		return;
	}

	const glm::vec2 ownSize = {allocatedRect.w, allocatedRect.h};

	for (size_t i = 0; i < children.size(); ++i) {
		Widget *child = children[i].get();
		const auto &[anchor, pivot, offset] = anchorParams[i];

		const BoxConstraints measureBox = BoxConstraints::loose(ownSize);
		glm::vec2 childSize = child->measure(ctx, measureBox);

		const glm::vec2 anchorPos = ownSize * anchor;
		const glm::vec2 pivotPos = childSize * pivot;
		const glm::vec2 finalPos = anchorPos - pivotPos + offset;

		child->layout(ctx, {finalPos.x, finalPos.y, childSize.x, childSize.y});
	}
}
} // namespace Ui
