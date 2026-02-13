#include "LayoutEngine.hpp"

#include "Base/Widget.hpp"

#include <ranges>
#include <span>

namespace {
using namespace Ui::LayoutEngine;
SDL_FRect alignBox(const SDL_FRect slot, const glm::vec2 size, const Alignment horizontal, const Alignment vertical)
{
	float x = slot.x;
	float y = slot.y;
	const float width = (horizontal == Alignment::Stretch) ? slot.w : size.x;
	const float height = (vertical == Alignment::Stretch) ? slot.h : size.y;

	if (horizontal == Alignment::Center)
		x += (slot.w - width) / 2.0f;
	if (horizontal == Alignment::End)
		x += (slot.w - width);

	if (vertical == Alignment::Center)
		y += (slot.h - height) / 2.0f;
	if (vertical == Alignment::End)
		y += (slot.h - height);

	return {x, y, width, height};
}

std::vector<float> resolveTracks(const std::vector<Track> &defs, const int uniformCount, const float availableSpace,
                                 float gap)
{
	std::vector<float> results;

	if (defs.empty()) {
		const float size = (availableSpace - ((uniformCount - 1) * gap)) / uniformCount;
		results.assign(uniformCount, std::max(0.0f, size));
		return results;
	}

	float totalFlex = 0.0f;
	float usedFixed = 0.0f;

	for (const auto &[type, value] : defs) {
		if (type == Track::Fixed)
			usedFixed += value;
		else
			totalFlex += value;
	}

	const float flexSpace = std::max(0.0f, availableSpace - usedFixed - ((defs.size() - 1) * gap));
	const float flexUnit = (totalFlex > 0) ? (flexSpace / totalFlex) : 0.0f;

	results.reserve(defs.size());
	for (const auto &[type, value] : defs) {
		if (type == Track::Fixed)
			results.push_back(value);
		else
			results.push_back(value * flexUnit);
	}
	return results;
}
} // namespace

namespace Ui::LayoutEngine {

const AnchorParams AnchorParams::center = {{0.5f, 0.5f}, {0.5f, 0.5f}, {0.0f, 0.0f}};
const AnchorParams AnchorParams::topLeft = {{0.0f, 0.0f}, {0.0f, 0.0f}, {10.0f, 10.0f}};
const AnchorParams AnchorParams::topRight = {{1.0f, 0.0f}, {1.0f, 0.0f}, {-10.0f, 10.0f}};
const AnchorParams AnchorParams::topCenter = {{0.5f, 0.0f}, {0.5f, 0.0f}, {0.0f, 10.0f}};
const AnchorParams AnchorParams::bottomCenter = {{0.5f, 1.0f}, {0.5f, 1.0f}, {0.0f, -10.0f}};

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

void solveLinear(std::span<LayoutItem> items, const SDL_FRect &parentRect, const LinearConfig &config)
{
	if (items.empty())
		return;

	const bool isVert = config.orientation == Orientation::Vertical;
	const float axisSize = isVert ? parentRect.h : parentRect.w;
	const float crossSize = isVert ? parentRect.w : parentRect.h;

	float usedFixed = 0.0f;
	float totalWeight = 0.0f;

	for (const auto &item : items) {
		if (item.weight > 0.0f) {
			totalWeight += item.weight;
		} else {
			usedFixed += isVert ? item.idealSize.y : item.idealSize.x;
		}
	}

	const float totalSpacing = std::max(0.0f, static_cast<float>(items.size() - 1)) * config.spacing;
	usedFixed += totalSpacing;

	const float paddingMain =
	    isVert ? config.padding.top + config.padding.bottom : config.padding.left + config.padding.right;
	const float remainingSpace = std::max(0.0f, axisSize - usedFixed - paddingMain);
	const float flexUnit = totalWeight > 0.0f ? remainingSpace / totalWeight : 0.0f;

	float cursor = isVert ? config.padding.top : config.padding.left;

	for (auto &item : items) {
		float mainLen = 0.0f;
		float crossLen = 0.0f;

		if (item.weight > 0.0f) {
			mainLen = item.weight * flexUnit;
		} else {
			mainLen = isVert ? item.idealSize.y : item.idealSize.x;
		}

		const Alignment crossAlign = isVert ? item.hAlign : item.vAlign;
		float itemCrossIdeal = isVert ? item.idealSize.x : item.idealSize.y;
		const float paddingCross =
		    isVert ? config.padding.left + config.padding.right : config.padding.top + config.padding.bottom;
		if (crossAlign == Alignment::Stretch) {
			crossLen = crossSize - paddingCross;
		} else {
			crossLen = std::min(itemCrossIdeal, crossSize - paddingCross);
		}

		const float crossStart = isVert ? config.padding.left : config.padding.top;
		float crossOffset = 0.0f;

		const float availableCross = crossSize - paddingCross;

		if (crossAlign == Alignment::Center) {
			crossOffset = (availableCross - crossLen) / 2.0f;
		} else if (crossAlign == Alignment::End) {
			crossOffset = availableCross - crossLen;
		}

		SDL_FRect finalRect;
		if (isVert) {
			finalRect = {parentRect.x + crossStart + crossOffset, parentRect.y + cursor, crossLen, mainLen};
		} else {
			finalRect = {parentRect.x + cursor, parentRect.y + crossStart + crossOffset, mainLen, crossLen};
		}

		item.calculatedBounds = finalRect;

		cursor += mainLen + config.spacing;
	}
}

void solveStack(const std::span<LayoutItem> items, const SDL_FRect &parentRect)
{
	for (auto &item : items) {
		if (item.anchor) {
			const SDL_FRect relative = positionAnchored({parentRect.w, parentRect.h}, item.idealSize, *item.anchor);
			item.calculatedBounds = {parentRect.x + relative.x, parentRect.y + relative.y, relative.w, relative.h};
		} else {
			item.calculatedBounds = alignBox(parentRect, item.idealSize, item.hAlign, item.vAlign);
		}
	}
}

void solveGrid(const std::span<LayoutItem> items, const SDL_FRect &parentRect, const GridConfig &cfg)
{
	const float availW = parentRect.w - cfg.padding.left - cfg.padding.right;
	const std::vector<float> colWidths = resolveTracks(cfg.colDefs, cfg.uniformCols, availW, cfg.gap);

	const float availH = parentRect.h - cfg.padding.top - cfg.padding.bottom;
	const std::vector<float> rowHeights = resolveTracks(cfg.rowDefs, cfg.uniformRows, availH, cfg.gap);

	for (auto &item : items) {
		float x = parentRect.x + cfg.padding.left;
		for (int i = 0; i < item.col; ++i)
			x += colWidths[i] + cfg.gap;

		float w = -cfg.gap;
		for (int i = 0; i < item.spanX; ++i)
			w += colWidths[item.col + i] + cfg.gap;

		float y = parentRect.y + cfg.padding.top;
		for (int i = 0; i < item.row; ++i)
			y += rowHeights[i] + cfg.gap;

		float h = -cfg.gap;
		for (int i = 0; i < item.spanY; ++i)
			h += rowHeights[item.row + i] + cfg.gap;

		item.calculatedBounds = alignBox({x, y, w, h}, item.idealSize, item.hAlign, item.vAlign);
	}
}
} // namespace Ui::LayoutEngine