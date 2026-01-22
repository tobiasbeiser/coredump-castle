


#include "LinearLayout.hpp"

#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/Gui/Core/UiContext.hpp"
#include "Engine/Graphics/Gui/Core/UiTypes.hpp"

namespace Ui
{
	LinearLayout::LinearLayout(const Orientation orient, const float gap) : orientation(orient), spacing(gap)
	{
		setSizeMode(SizeMode::MatchParent, SizeMode::MatchParent);
	}

	void LinearLayout::setCrossAxisAlignment(const CrossAxisAlignment align)
	{
		crossAxisAlignment = align;
		invalidate();
	}

	void LinearLayout::setSpacing(const float gap)
	{ // TODO: change return type to LinearLayout& (return *this;) after CRTP implementation
		spacing = gap;
	}

	InternalDimensions LinearLayout::measureFixedChildren(const bool isVert, const BoxConstraints constraints,
														  const UIContext &ctx) const
	{
		InternalDimensions dims;
		for (const auto &child : getChildren()) {
			if (const float weight = child->getLayoutWeight(); weight > 0.0f) {
				dims.totalFlex += weight; // add up weights of flexible items
			} else {

				const bool isMatchParentCross = isVert ? (child->getWidthMode() == SizeMode::MatchParent)
													   : (child->getHeightMode() == SizeMode::MatchParent);

				float crossMin = 0.0f;
				const float crossMax = isVert ? constraints.maxWidth : constraints.maxHeight;
				if (crossAxisAlignment == CrossAxisAlignment::Stretch || isMatchParentCross) {
					if ((isVert && constraints.hasBoundedHeight()) || (!isVert && constraints.hasBoundedWith()))
						crossMin = crossMax;
				}

				const BoxConstraints childConstraints = isVert ? BoxConstraints(crossMin, crossMax, 0.0f, kInfinity)
															   : BoxConstraints(0.0f, kInfinity, crossMin, crossMax);

				glm::vec2 childSize = child->measure(ctx, childConstraints);

				dims.fixedSpace += (isVert ? childSize.y : childSize.x);

				dims.maxCrossAxis = std::max(dims.maxCrossAxis, isVert ? childSize.x : childSize.y);
			}
		}

		// add spacing
		if (!getChildren().empty()) {
			dims.fixedSpace += spacing * static_cast<float>(getChildren().size() - 1);
		}

		return dims;
	}

	void LinearLayout::measureFlexibleChildren(InternalDimensions &tmpDimensions, const bool isVert,
											   const BoxConstraints constraints, const UIContext &ctx) const
	{
		const float maxAvailable = isVert ? constraints.maxHeight : constraints.maxWidth;
		const float remainingSpace = std::max(0.0f, maxAvailable - tmpDimensions.fixedSpace);

		for (const auto &child : getChildren()) {

			if (child->getLayoutWeight() <= 0.0f)
				continue; // skip fixed items

			const float share =
				child->getLayoutWeight() / tmpDimensions.totalFlex * remainingSpace; // calculate portion of remaining space

			float crossMin = 0.0f;
			const float crossMax = isVert ? constraints.maxWidth : constraints.maxHeight;
			if (crossAxisAlignment == CrossAxisAlignment::Stretch) {
				if ((isVert && constraints.hasBoundedHeight()) || (!isVert && constraints.hasBoundedWith()))
					crossMin = crossMax;
			}

			// create flex constraints
			BoxConstraints flexConstraints;
			if (isVert) {
				flexConstraints = {crossMin, crossMax, share, share};
			} else {
				flexConstraints = {share, share, crossMin, crossMax};
			}

			glm::vec2 childSize = child->measure(ctx, flexConstraints);
			tmpDimensions.maxCrossAxis = std::max(tmpDimensions.maxCrossAxis, isVert ? childSize.x : childSize.y);
		}
	}

	glm::vec2 LinearLayout::performMeasure(const UIContext &ctx, const BoxConstraints constraints) const
	{
		const bool isVert = (orientation == Orientation::Vertical);

		// resolve fixed space
		InternalDimensions dimensions = measureFixedChildren(isVert, constraints, ctx);
		// resolve flexible space
		measureFlexibleChildren(dimensions, isVert, constraints, ctx);

		const float totalMain = dimensions.fixedSpace + (dimensions.totalFlex > 0 ? dimensions.remainingSpace : 0);
		return isVert ? glm::vec2{dimensions.maxCrossAxis, totalMain} : glm::vec2{totalMain, dimensions.maxCrossAxis};
	}

	void LinearLayout::layout(const UIContext &ctx, const SDL_FRect allocatedRect)
	{
		Widget::layout(ctx, allocatedRect);

		const bool isVert = (orientation == Orientation::Vertical);
		float cursor = 0;

		// recalculate size based on actual size
		float totalFlex = 0.0f;
		float usedFixed = 0.0f;
		for (const auto &child : getChildren()) {
			if (child->getLayoutWeight() > 0)
				totalFlex += child->getLayoutWeight();
			else {
				// use cached size
				glm::vec2 size = child->getMeasuredSize();
				usedFixed += (isVert ? size.y : size.x);
			}
		}

		if (!getChildren().empty())
			usedFixed += spacing * (getChildren().size() - 1);

		const float availableLen = isVert ? allocatedRect.h : allocatedRect.w;
		const float remaining = std::max(0.0f, availableLen - usedFixed);

		for (const auto &child : getChildren()) {
			glm::vec2 childSize = child->getMeasuredSize();
			float mainAxisLen = 0.0f;
			if (const float weight = child->getLayoutWeight(); weight > 0.0f) {
				mainAxisLen = (weight / totalFlex) * remaining;
			} else {
				mainAxisLen = isVert ? childSize.y : childSize.x;
			}

			float crossAxisLen = isVert ? childSize.x : childSize.y;
			float crossAxisPos = 0;
			const float crossAxisAvail = isVert ? allocatedRect.w : allocatedRect.h;

			const bool isMatchParentCross = isVert ? (child->getWidthMode() == SizeMode::MatchParent)
												   : (child->getHeightMode() == SizeMode::MatchParent);

			if (crossAxisAlignment == CrossAxisAlignment::Stretch || isMatchParentCross) {
				crossAxisLen = crossAxisAvail;
			} else if (crossAxisAlignment == CrossAxisAlignment::Center) {
				crossAxisPos += (crossAxisAvail - crossAxisLen) / 2.0f;
			} else if (crossAxisAlignment == CrossAxisAlignment::End) {
				crossAxisPos += crossAxisAvail - crossAxisLen;
			}
			SDL_FRect childRect;

			if (isVert) {
				childRect = {crossAxisPos, cursor, crossAxisLen, mainAxisLen};
			} else {
				childRect = {cursor, crossAxisPos, mainAxisLen, crossAxisLen};
			}
			cursor += mainAxisLen + spacing;

			child->layout(ctx, childRect);
		}
	}

	void LinearLayout::setOrientation(const Orientation newOrientation)
	{
		orientation = newOrientation;
	}
}
