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
	const auto [x, y, w, h] = absoluteRect(parentPos);


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

	const glm::vec2 innerAvailable = {availableSize.x - buttonPadding,
	                                  availableSize.y - buttonPadding};

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
	const SDL_FRect absRect = absoluteRect(parentAbsPos);
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
	const SDL_FRect r = absoluteRect(parentAbsPos);

	ctx.renderer.drawRectShape(r, fillCol, outlineCol);

	if (auto *label = dynamic_cast<Label *>(content.get())) {
		label->setColor(textCol);
	}

	content->draw(ctx, {r.x, r.y});
}

void Frame::addWidget(std::unique_ptr<Widget> widget)
{
	widgets.push_back(std::move(widget));
}

bool Frame::handleEvent(const glm::vec2 mousePos, const glm::vec2 parentAbsPos, const SDL_Event &event)
{
	const SDL_FPoint mousePt = {mousePos.x, mousePos.y};
	const SDL_FRect absRect = absoluteRect(parentAbsPos);

	if (SDL_PointInRectFloat(&mousePt, &absRect)) {
	}

	for (const auto &child : std::ranges::reverse_view(widgets)) {
		if (child->handleEvent(mousePos, absolutePosition(parentAbsPos), event)) {
			return true;
		}
	}
	return false;
}

void Frame::draw(const UIContext &ctx, const glm::vec2 parentAbsPos)
{

	const SDL_FRect drawRect = absoluteRect(parentAbsPos);

	// TODO: make frame be drawable with individual colors
	const auto &fs = ctx.theme.frame;
	ctx.renderer.drawRectShape(drawRect, fs.fill, fs.outline);

	for (const auto &widget : widgets)
		widget->draw(ctx, absolutePosition(parentAbsPos));
}

glm::vec2 VBox::getPreferredSize(const UIContext &ctx, const glm::vec2 availableSize)
{

	float maxChildWidth = 0.0f;
	float totalHeight = padding * 2.0f;

	// Subtract padding from available space for children
	const glm::vec2 innerAvailable = {availableSize.x - (padding * 2), availableSize.y - (padding * 2)};

	for (const auto &child : children) {
		glm::vec2 childSize = child->getPreferredSize(ctx, innerAvailable);
		maxChildWidth = std::max(maxChildWidth, childSize.x);
		totalHeight += childSize.y;
	}

	if (!children.empty()) {
		totalHeight += (children.size() - 1) * spacing;
	}

	const glm::vec2 contentSize = {maxChildWidth + (padding * 2), totalHeight};
	return resolveSize(contentSize, availableSize);
}

void VBox::layout(const UIContext &ctx, SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);

	// 2. Layout children
	float currentY = padding;
	float innerWidth = allocatedRect.w - (padding * 2);

	for (auto &child : children) {
		// Ask child for height again, now that we know our exact width
		glm::vec2 childPref = child->getPreferredSize(ctx, {innerWidth, allocatedRect.h});

		// Calculate child rect
		SDL_FRect childRect;
		childRect.y = currentY;
		childRect.w = child->getWidthMode() == SizeMode::MatchParent ? innerWidth : childPref.x;
		childRect.h = childPref.y;
		childRect.x = padding + (innerWidth - childRect.w) / 2.0f;

		child->layout(ctx, childRect);

		currentY += childRect.h + spacing;
	}
}

bool VBox::handleEvent(const glm::vec2 mousePos, const glm::vec2 parentAbsPos, const SDL_Event &event)
{
	// Reverse iteration for event bubbling (top-most child first)
	for (const auto &child : std::ranges::reverse_view(children)) {
		if (child->handleEvent(mousePos, absolutePosition(parentAbsPos), event))
			return true;
	}
	return false;
}

void VBox::draw(const UIContext &context, const glm::vec2 parentAbsPos)
{

	for (const auto &child : children) {
		child->draw(context, absolutePosition(parentAbsPos));
	}
}

void GuiManager::updateLayout(const UIContext &context, const float screenWidth, const float screenHeight) const
{
	if (!rootWidget)
		return;
	// TODO: check if this is even necessary

	// 1. Measure Pass: Ask root how big it wants to be with full screensize available
	glm::vec2 available = {screenWidth, screenHeight};
	glm::vec2 pref = rootWidget->getPreferredSize(context, available);

	// 2. Layout Pass: Set the root to its preferred size (or fullscreen)
	SDL_FRect finalRect = {0.0f, 0.0f, pref.x, pref.y};

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
