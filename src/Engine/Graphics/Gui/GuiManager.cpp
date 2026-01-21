#include "GuiManager.hpp"

#include <ranges>

#include "Engine/Graphics/Renderer.hpp"

using namespace Ui;

glm::vec2 Widget::measure(const UIContext &ctx, BoxConstraints constraints) const
{
	if (widthMode == SizeMode::Fixed) {
		constraints.minWidth = fixedSize.x;
		constraints.maxWidth = fixedSize.x;
	}

	if (heightMode == SizeMode::Fixed) {
		constraints.minHeight = fixedSize.y;
		constraints.maxHeight = fixedSize.y;
	}

	if (cachedSize.has_value() && lastConstraints.has_value() && constraints.minWidth == lastConstraints->minWidth
	    && constraints.maxWidth == lastConstraints->maxWidth && constraints.minHeight == lastConstraints->minHeight
	    && constraints.maxHeight == lastConstraints->maxHeight && !isDirty) {
		return cachedSize.value();
	}

	glm::vec2 size = performMeasure(ctx, constraints);
	size = constraints.constrain(size);

	cachedSize = size;
	lastConstraints = constraints;
	isDirty = false;
	return size;
}

glm::vec2 Widget::getMeasuredSize() const
{
	// helper, since we don't have box constraints in layout phase and therefore can't call measure()
	return cachedSize.value_or(glm::vec2{0.0f, 0.0f});
}

void Widget::layout(const UIContext &ctx, const SDL_FRect allocatedRect)
{
	bounds = allocatedRect;
}

void Widget::invalidate()
{
	isDirty = true;
}

void Widget::setLayoutWeight(const float weight)
{
	layoutWeight = weight;
}

float Widget::getLayoutWeight() const
{
	return layoutWeight;
}

void Widget::setSizeMode(const SizeMode w, const SizeMode h)
{
	widthMode = w;
	heightMode = h;
	invalidate();
}

void Widget::setFixedSize(const glm::vec2 size)
{
	fixedSize = size;
	widthMode = SizeMode::Fixed;
	heightMode = SizeMode::Fixed;
}

SDL_FRect Widget::getBounds() const
{
	return bounds;
}

SizeMode Widget::getWidthMode() const
{
	return widthMode;
}

SizeMode Widget::getHeightMode() const
{
	return heightMode;
}

glm::vec2 Widget::getRelativePosition() const
{
	return {bounds.x, bounds.y};
}

Label::Label(const std::string_view labelText, TTF_Font *font) : labelText(labelText), font(font)
{
	setSizeMode(SizeMode::WrapContent, SizeMode::WrapContent);
}

void Label::layout(const UIContext &ctx, const SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);
}

void Label::draw(const UIContext &ctx, const glm::vec2 parentOffset) const
{
	if (labelText.empty())
		return;

	ensureTextObject(ctx);
	if (colorDirty) {
		TTF_SetTextColor(textObject.get(), color.r, color.g, color.b, color.a);
		colorDirty = false;
	}

	// auto rect = getScreenRect();
	// SDL_SetRenderDrawColor(ctx.renderer->getNativeRenderer(), 255, 0, 0, 0);
	// SDL_RenderRect(ctx.renderer->getNativeRenderer(), &rect);

	// TODO: solve subpixel rendering prevention
	const auto absolutePosition = parentOffset + getRelativePosition();
	ctx.renderer->drawText(textObject.get(), absolutePosition.x, absolutePosition.y);
}

void Label::setColor(const SDL_Color newColor)
{
	if (newColor == color)
		return;

	color = newColor;
	colorDirty = true;
}

void Label::setText(const std::string_view newText)
{
	if (labelText == newText)
		return;

	labelText = newText;

	if (textObject) {
		TTF_SetTextString(textObject.get(), labelText.c_str(), labelText.length());
	}

	invalidate();
}

glm::vec2 Label::performMeasure(const UIContext &ctx, const BoxConstraints constraints) const
{
	if (labelText.empty())
		return {0, 0};

	ensureTextObject(ctx);

	int w = 0, h = 0;

	// check if we need to wrap text
	if (constraints.hasBoundedWith()) {
		TTF_SetTextWrapWidth(textObject.get(), static_cast<int>(constraints.maxWidth));
	} else {
		TTF_SetTextWrapWidth(textObject.get(), 0);
	}

	TTF_GetTextSize(textObject.get(), &w, &h);
	return {static_cast<float>(w), static_cast<float>(h)};
}

void Label::ensureTextObject(const UIContext &ctx) const
{
	if (!textObject) {
		textObject = ctx.renderer->createText(font, labelText);
		colorDirty = true;
	}
}

Button::Button(std::string_view label, TTF_Font *font, const std::function<void()> &onClick) : callback(onClick)
{
	auto lbl = std::make_unique<Label>(label, font);
	textLabel = lbl.get();
	content = std::move(lbl);
	setSizeMode(SizeMode::WrapContent, SizeMode::WrapContent);
}

Button::Button(std::unique_ptr<Widget> customContent, std::function<void()> onClick)
    : content(std::move(customContent)), callback(std::move(onClick))
{
	setSizeMode(SizeMode::WrapContent, SizeMode::WrapContent);
}

glm::vec2 Button::performMeasure(const UIContext &ctx, const BoxConstraints constraints) const
{
	const float xPad = ctx.theme.button.widthPadding * 2.0f;
	const float yPad = ctx.theme.button.heightPadding * 2.0f;

	BoxConstraints childConstraints = constraints;
	childConstraints.maxWidth = std::max(0.0f, constraints.maxWidth - xPad);
	childConstraints.maxHeight = std::max(0.0f, constraints.maxHeight - yPad);
	childConstraints.minWidth = std::min(0.0f, constraints.minWidth - xPad);
	childConstraints.minHeight = std::min(0.0f, constraints.minHeight - yPad);

	const glm::vec2 contentSize = content->measure(ctx, childConstraints);
	return {contentSize.x + xPad, contentSize.y + yPad};
}

void Button::layout(const UIContext &ctx, SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);

	const auto constraints = BoxConstraints::loose({allocatedRect.w, allocatedRect.h});
	const glm::vec2 contentSize = content->measure(ctx, constraints);

	float offsetX = 0.0f;
	float offsetY = 0.0f;

	offsetX = getWidthMode() == SizeMode::WrapContent ? ctx.theme.button.widthPadding
	                                                  : (allocatedRect.w - contentSize.x) / 2.0f;

	offsetY = getHeightMode() == SizeMode::WrapContent ? ctx.theme.button.heightPadding
	                                                   : (allocatedRect.h - contentSize.y) / 2.0f;

	const SDL_FRect contentRect = {offsetX, offsetY, contentSize.x, contentSize.y};

	content->layout(ctx, contentRect);
}

bool Button::handleEvent(const glm::vec2 localMousePos, const SDL_Event &event)
{
	const SDL_FRect bounds = getBounds();
	const SDL_FPoint mousePt = {localMousePos.x, localMousePos.y};
	const bool isInside = SDL_PointInRectFloat(&mousePt, &bounds);

	if (event.type == SDL_EVENT_MOUSE_MOTION) {
		isHovered = isInside;
		return isHovered;
	}

	if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
		if (isHovered) {
			isPressed = true;
			return true;
		}
	}

	if (event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT) {
		const bool wasPressed = isPressed;
		isPressed = false;

		if (wasPressed && isInside && callback) {
			callback();
			return true;
		}
	}

	return false;
}

void Button::draw(const UIContext &ctx, const glm::vec2 parentOffset) const
{
	const WidgetState state = isDisabled  ? WidgetState::Disabled
	                          : isPressed ? WidgetState::Pressed
	                          : isHovered ? WidgetState::Hovered
	                                      : WidgetState::Normal;

	const auto &[fillCol, outlineCol, textCol] = ctx.theme.button.paintFor(state);
	SDL_FRect rect = getScreenRect(parentOffset);

	ctx.renderer->drawRectShape(rect, fillCol, outlineCol);

	if (auto *label = dynamic_cast<Label *>(content.get())) {
		label->setColor(textCol);
	}
	if (content) {
		content->draw(ctx, {rect.x, rect.y});
	}
}

void Button::setButtonDisabled(const bool disabled)
{
	isDisabled = disabled;
}

void Container::addWidget(std::unique_ptr<Widget> widget)
{
	children.push_back(std::move(widget));
}

bool Container::handleEvent(const glm::vec2 localMousePos, const SDL_Event &event)
{
	const auto childMousePos = localMousePos - getRelativePosition();

	// Reverse iteration for event bubbling (top-most child first)
	for (const auto &child : std::ranges::reverse_view(getChildren())) {
		if (child->handleEvent(childMousePos, event))
			return true;
	}
	return false;
}

void Container::draw(const UIContext &context, const glm::vec2 parentOffset) const
{
	const auto rect = getScreenRect(parentOffset);

	SDL_SetRenderDrawColor(context.renderer->getNativeRenderer(), 255, 0, 0, 0);
	SDL_RenderRect(context.renderer->getNativeRenderer(), &rect);

	const auto selfOffset = parentOffset + getRelativePosition();
	for (const auto &child : getChildren()) {
		child->draw(context, selfOffset);
	}
}

const std::vector<std::unique_ptr<Widget>>& Container::getChildren() const
{
	return children;
}

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

void GuiManager::setRoot(std::unique_ptr<Widget> root)
{
	rootWidget = std::move(root);
}

void GuiManager::updateLayout(const UIContext &context, const float screenWidth, const float screenHeight) const
{
	if (!rootWidget)
		return;
	// // TODO: check if this is even necessary
	const BoxConstraints screenConstraints = BoxConstraints::tight(screenWidth, screenHeight);

	rootWidget->measure(context, screenConstraints);

	// 2. Layout Pass: Set the root to its preferred size (or fullscreen)
	const SDL_FRect finalRect = {0.0f, 0.0f, screenWidth, screenHeight};

	rootWidget->layout(context, finalRect);
}

void GuiManager::handleEvent(const UIContext &ctx, const SDL_Event &event) const
{
	if (event.type != SDL_EVENT_MOUSE_MOTION && event.type != SDL_EVENT_MOUSE_BUTTON_DOWN
	    && event.type != SDL_EVENT_MOUSE_BUTTON_UP) {
		return;
	}

	float mx = 0, my = 0;

	// Extract raw Window coordinates based on event type
	if (event.type == SDL_EVENT_MOUSE_MOTION) {
		mx = event.motion.x;
		my = event.motion.y;
	} else {
		mx = event.button.x;
		my = event.button.y;
	}

	// CONVERT to Logical Coordinates
	const glm::vec2 mouseLogicalPos = ctx.renderer->windowToLogical(mx, my);

	if (rootWidget)
		rootWidget->handleEvent(mouseLogicalPos, event);
}

void GuiManager::render(const UIContext &ctx) const
{
	if (rootWidget)
		rootWidget->draw(ctx, {0, 0});
}
