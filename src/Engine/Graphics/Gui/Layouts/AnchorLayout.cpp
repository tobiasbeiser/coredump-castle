
#include "AnchorLayout.hpp"

namespace Ui {
// --- Presets ---
const AnchorParams AnchorParams::center = {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.0f, 0.0f}};
const AnchorParams AnchorParams::topLeft = {{0.0f, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f}};
const AnchorParams AnchorParams::topRight = {{1.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f}};
const AnchorParams AnchorParams::bottomCenter = {{0.5f, 1.0f}, {0.5f, 1.0f}, {0.0f, 0.0f}};

void AnchorLayout::addChild(std::unique_ptr<Widget> widget, const AnchorParams params)
{
	// We must push params before moving the widget to base,
	// to ensure indices match if base throws (though unlikely here)
	anchorParams.push_back(params);
	addWidget(std::move(widget));
}

glm::vec2 AnchorLayout::performMeasure(const UIContext &ctx, const BoxConstraints constraints) const
{
	// AnchorLayout is essentially a "Stack". It adapts to the biggest child
	// OR it fills the parent if MatchParent is set.

	float maxWidth = 0.0f;
	float maxHeight = 0.0f;

	// In a stack, constraints are usually loosened.
	// We let children be as big as the parent allows.
	auto looseConstraints = constraints;
	looseConstraints.minWidth = 0;
	looseConstraints.minHeight = 0;

	for (const auto &child : getChildren()) {
		glm::vec2 size = child->measure(ctx, looseConstraints);
		maxWidth = std::max(maxWidth, size.x);
		maxHeight = std::max(maxHeight, size.y);
	}

	return {maxWidth, maxHeight};
}

void AnchorLayout::layout(const UIContext &ctx, SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);

	const auto &children = getChildren();

	if (children.size() != anchorParams.size()) {
		// In a real engine, log an error here.
		return;
	}

	for (size_t i = 0; i < children.size(); ++i) {
		Widget *child = children[i].get();
		const AnchorParams &params = anchorParams[i];

		// 1. Get the child's desired size
		// We give it the full area to measure against, but 0-infinity bounds
		BoxConstraints measureBox = BoxConstraints::loose({allocatedRect.w, allocatedRect.h});
		glm::vec2 childSize = child->measure(ctx, measureBox);

		// 2. Calculate Position
		// Parent Anchor Point (e.g., Center of Screen)
		float anchorX = allocatedRect.w * params.anchor.x;
		float anchorY = allocatedRect.h * params.anchor.y;

		// Child Pivot Point (e.g., Center of Label)
		float pivotX = childSize.x * params.pivot.x;
		float pivotY = childSize.y * params.pivot.y;

		// Final Calculation
		float destX = anchorX - pivotX + params.offset.x;
		float destY = anchorY - pivotY + params.offset.y;

		child->layout(ctx, {destX, destY, childSize.x, childSize.y});
	}
}
} // namespace Ui
