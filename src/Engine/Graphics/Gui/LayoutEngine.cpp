#include "LayoutEngine.hpp"

#include "Base/Widget.hpp"
#include "Layouts/AnchorLayout.hpp"

#include <ranges>
#include <span>

namespace Ui::LayoutEngine {

SDL_FRect positionAnchored(const glm::vec2 allocatedSize, const glm::vec2 ownSize, const AnchorParams &anchorParams)
{
	const auto [anchor, pivot, offset] = anchorParams;

	const glm::vec2 anchorPos = allocatedSize * anchor;
	const glm::vec2 pivotPos = ownSize * pivot;
	const glm::vec2 finalPos = anchorPos - pivotPos + offset;
	return {finalPos.x, finalPos.y, ownSize.x, ownSize.y};
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

void layoutAnchored(Widget *widget, const UIContext &ctx, const SDL_FRect allocatedRect, const AnchorParams &params)
{
	const glm::vec2 parentSize = {allocatedRect.w, allocatedRect.h};

	const BoxConstraints measureBox = BoxConstraints::loose(parentSize);
	const glm::vec2 widgetSize = widget->measure(ctx, measureBox);

	widget->layout(ctx, positionAnchored(parentSize, widgetSize, params));
}


// The Solver Function
// Calculates absolute positions and bakes them into the widgets immediately.
inline void solveLinear(std::span<LayoutItem> items, const SDL_FRect& parentRect, const LinearConfig& config) {
    if (items.empty()) return;

    const bool isVert = config.orientation == Orientation::Vertical;
    const float axisSize = isVert ? parentRect.h : parentRect.w;
    const float crossSize = isVert ? parentRect.w : parentRect.h;

    // 1. Calculate Fixed Space and Total Weight using Ranges
    auto fixedItems = items | std::views::filter([](const auto& i){ return i.weight <= 0.0f; });
    auto flexItems  = items | std::views::filter([](const auto& i){ return i.weight > 0.0f; });

    float usedFixed = 0.0f;
    for (const auto& item : fixedItems) {
        usedFixed += isVert ? item.idealSize.y : item.idealSize.x;
    }

    // Add spacing gap overhead
    const float totalSpacing = std::max(0.0f, float(items.size() - 1)) * config.spacing;
    usedFixed += totalSpacing;

    // 2. Calculate Flexible Space Unit
    float totalWeight = 0.0f;
    for (const auto& item : flexItems) totalWeight += item.weight;

    const float remainingSpace = std::max(0.0f, axisSize - usedFixed - (isVert ? config.padding.y * 2 : config.padding.x * 2));
    const float flexUnit = (totalWeight > 0) ? (remainingSpace / totalWeight) : 0.0f;

    // 3. Position Items (Baking)
    float cursor = isVert ? config.padding.y : config.padding.x;

    for (auto& item : items) {
        float mainLen = 0.0f;
        float crossLen = 0.0f;

        // Determine Main Axis Length
        if (item.weight > 0.0f) {
            mainLen = item.weight * flexUnit;
        } else {
            mainLen = isVert ? item.idealSize.y : item.idealSize.x;
        }

        // Determine Cross Axis Length & Position
        float itemCrossIdeal = isVert ? item.idealSize.x : item.idealSize.y;

        if (item.align == Alignment::Stretch) {
            crossLen = crossSize - (isVert ? config.padding.x * 2 : config.padding.y * 2);
        } else {
            crossLen = itemCrossIdeal; // Respect ideal size
        }

        float crossOffset = isVert ? config.padding.x : config.padding.y;

        // Handle Alignment logic
        if (item.align == Alignment::Center) {
             crossOffset += (crossSize - crossLen) / 2.0f;
        } else if (item.align == Alignment::End) {
             crossOffset += (crossSize - crossLen);
        }

        // 4. Bake Final Rect into Widget
        SDL_FRect finalRect;
        if (isVert) {
            finalRect = { parentRect.x + crossOffset, parentRect.y + cursor, crossLen, mainLen };
        } else {
            finalRect = { parentRect.x + cursor, parentRect.y + crossOffset, mainLen, crossLen };
        }

        item.widget->setBounds(finalRect);

        cursor += mainLen + config.spacing;
    }
}

} // namespace Ui::LayoutEngine