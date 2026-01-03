#include "GuiManager.hpp"

#include <ranges>

#include "Engine/Graphics/Renderer.hpp"

glm::vec2 Label::getPreferredSize(const UIContext &ctx, const glm::vec2 availableSize)
{

	if (labelText.empty())
		return resolveSize({0, 0}, availableSize);

	if (!textObject)
		textObject = ctx.renderer.createText(font, labelText);
	// TODO: Figure out how to handle text formatting. Can't format text before creating it, i.e. first getPreferredSize
	//  Idea: cache format calls as templates/lambdas and trigger them after creating text? see:
	//  TTF_SetTextWrapWidth(textObject.get(), wrapWidth); TTF_SetTextWrapWhitespaceVisible(textObject.get(), visible);

	int w = 0, h = 0;
	TTF_GetTextSize(textObject.get(), &w, &h);

	return {static_cast<float>(w), static_cast<float>(h)};
}

void Label::layout(const UIContext &ctx, const SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);
}

void Label::draw(const UIContext &ctx, const glm::vec2 parentPos)
{
	const auto [x, y, w, h] = getAbsoluteRect(parentPos);


	// TODO: solve subpixel rendering prevention
	ctx.renderer.drawTextAt(textObject.get(), x, y, color);
}

Button::Button(std::string_view label, TTF_Font *font, const std::function<void()> &onClick) : callback(onClick)
{
	content = std::make_unique<Label>(label, font);
	setSizeMode(SizeMode::WrapContent, SizeMode::WrapContent);
}

Button::Button(std::unique_ptr<Widget> customContent, std::function<void()> onClick)
    : content(std::move(customContent)), callback(std::move(onClick))
{
	setSizeMode(SizeMode::WrapContent, SizeMode::WrapContent);
}

glm::vec2 Button::getPreferredSize(const UIContext &ctx, const glm::vec2 availableSize)
{
	const auto buttonPadding = ctx.theme.button.padding * 2;

	const glm::vec2 innerAvailable = {
		std::max(0.0f, availableSize.x - buttonPadding),
		std::max(0.0f, availableSize.y - buttonPadding)
	};

	const glm::vec2 contentSize = content->getPreferredSize(ctx, innerAvailable);

	const glm::vec2 totalSize = {contentSize.x + buttonPadding,
	                             contentSize.y + buttonPadding};

	return resolveSize(totalSize, availableSize);
}

void Button::layout(const UIContext &ctx, SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);

	const glm::vec2 contentSize = content->getPreferredSize(ctx, {allocatedRect.w, allocatedRect.h});

	SDL_FRect contentRect;
	contentRect.w = contentSize.x;
	contentRect.h = contentSize.y;

	// Center content
	contentRect.x = (allocatedRect.w - contentSize.x) / 2.0f;
	contentRect.y = (allocatedRect.h - contentSize.y) / 2.0f;

	content->layout(ctx, contentRect);
}

bool Button::handleEvent(const glm::vec2 mousePos, const glm::vec2 parentAbsPos, const SDL_Event &event)
{
	const SDL_FRect absRect = getAbsoluteRect(parentAbsPos);
	const SDL_FPoint mousePt = {mousePos.x, mousePos.y};
	const bool isInside = SDL_PointInRectFloat(&mousePt, &absRect);

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

void Button::draw(const UIContext &ctx, const glm::vec2 parentAbsPos)
{

	const WidgetState s = isPressed ? WidgetState::Pressed : isHovered ? WidgetState::Hovered : WidgetState::Normal;

	const auto &[fillCol, outlineCol, textCol] = ctx.theme.button.paintFor(s);
	const SDL_FRect r = getAbsoluteRect(parentAbsPos);

	ctx.renderer.drawRectShape(r, fillCol, outlineCol);

	if (auto *label = dynamic_cast<Label *>(content.get())) {
		label->setColor(textCol);
	}

	content->draw(ctx, {r.x, r.y});
}

bool Container::handleEvent(const glm::vec2 mousePos, const glm::vec2 parentAbsPos, const SDL_Event &event)
{
	// Reverse iteration for event bubbling (top-most child first)
	for (const auto &child : std::ranges::reverse_view(children)) {
		if (child->handleEvent(mousePos, absolutePosition(parentAbsPos), event))
			return true;
	}
	return false;
}

void Container::draw(const UIContext &context, const glm::vec2 parentAbsPos)
{

	for (const auto &child : children) {
		child->draw(context, absolutePosition(parentAbsPos));
	}
}


glm::vec2 LinearLayout::getPreferredSize(const UIContext &ctx, const glm::vec2 availableSize)
{
	float primaryDim = 0.0f; // Height for Vertical, Width for Horizontal
	float secondaryDim = 0.0f; // Width for Vertical, Height for Horizontal

	const bool isVert = orientation == Orientation::Vertical;
	float availableX = 0.0f;
	float availableY = 0.0f;

	// calculate theoretically available space to single child
	if (isVert) {
		availableX = availableSize.x - padding * 2.f;
		availableY = (availableSize.y - padding * 2.f - (children.size() - 1) * spacing) / children.size();
	} else {
		availableX = (availableSize.x - padding * 2.f - (children.size() - 1) * spacing) / children.size();
		availableY = availableSize.y - padding * 2.f;
	}

	const bool isWrapSecondary = isVert ? (getWidthMode() == SizeMode::WrapContent)
										: (getHeightMode() == SizeMode::WrapContent);
	if (isWrapSecondary) {
		if (isVert) availableX = 0.0f;
		else        availableY = 0.0f;
	}

	const glm::vec2 innerAvail = {std::max(0.0f, availableX), std::max(0.0f, availableY)};

	for (const auto &child : children) {

		glm::vec2 childSize = child->getPreferredSize(ctx, innerAvail);

		if (isVert) {
			primaryDim += childSize.y;
			secondaryDim = std::max(secondaryDim, childSize.x);
		} else {
			primaryDim += childSize.x;
			secondaryDim = std::max(secondaryDim, childSize.y);
		}
	}

	if (!children.empty()) {
		primaryDim += (children.size() - 1) * spacing;
	}

	float finalW = (isVert ? secondaryDim : primaryDim) + padding * 2;
	float finalH = (isVert ? primaryDim : secondaryDim) + padding * 2;

	return resolveSize({finalW, finalH}, availableSize);
}

void LinearLayout::layout(const UIContext &ctx, SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);

	const bool isVert = (orientation == Orientation::Vertical);
	float cursor = padding;

	// Available space for children (absolute terms relative to this container)
	const float innerW = allocatedRect.w - padding * 2;
	const float innerH = allocatedRect.h - padding * 2;

	for (const auto &child : children) { // Ask child for height again, now that we know our exact width

		const glm::vec2 childPref = child->getPreferredSize(ctx, {innerW, innerH});

		// Calculate child rect
		SDL_FRect childRect;
		if (isVert) {
			childRect.x = padding;
			childRect.y = cursor;
			childRect.w = child->getWidthMode() == SizeMode::MatchParent ? innerW : childPref.x;
			childRect.h = childPref.y;

			if (child->getWidthMode() != SizeMode::MatchParent) {
				childRect.x += (innerW - childRect.w) / 2.0f;
			}

			cursor += childRect.h + spacing;
		} else
		{
			childRect.x = cursor;
			childRect.y = padding;
			childRect.w = childPref.x;
			childRect.h = child->getHeightMode() == SizeMode::MatchParent ? innerH : childPref.y;

			if (child->getHeightMode() != SizeMode::MatchParent) {
				childRect.y += (innerH - childRect.h) / 2.0f;
			}

			cursor += childRect.w + spacing;
		}
		child->layout(ctx, childRect);
	}
}

void GuiManager::updateLayout(const UIContext &context, const float screenWidth, const float screenHeight) const
{
	if (!rootWidget)
		return;
	// // TODO: check if this is even necessary
	//
	// // 1. Measure Pass: Ask root how big it wants to be with full screensize available
	// glm::vec2 available = {screenWidth, screenHeight};
	// glm::vec2 pref = rootWidget->getPreferredSize(context, available);

	// 2. Layout Pass: Set the root to its preferred size (or fullscreen)
	SDL_FRect finalRect = {0.0f, 0.0f, screenWidth, screenHeight};

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
	const glm::vec2 mouseLogicalPos = ctx.renderer.windowToLogical(mx, my);

	if (rootWidget)
		rootWidget->handleEvent(mouseLogicalPos, {0, 0}, event);
}

void GuiManager::render(const UIContext &ctx) const
{
	if (rootWidget)
		rootWidget->draw(ctx, {0, 0});
}
