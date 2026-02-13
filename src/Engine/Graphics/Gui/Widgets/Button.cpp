
#include "Button.hpp"

#include "Engine/Graphics/Gui/LayoutEngine.hpp"
#include "Label.hpp"

namespace Ui {
static constexpr Paint resolvePaint(const WidgetState state, const ButtonStyle &style)
{
	switch (state) {
	case WidgetState::Hovered:
		return style.hovered;
	case WidgetState::Pressed:
		return style.pressed;
	case WidgetState::Disabled:
		return style.disabled;
	case WidgetState::Normal:
	default:
		return style.normal;
	}
}

Button::Button(std::string_view label, TTF_Font *font, const std::function<void()> &onClick)
    : content(std::make_unique<Label>(label, font)), callback(onClick)
{
	if (!callback) {
		isDisabled = true;
	}
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

	const auto &[fillCol, outlineCol, textCol] = resolvePaint(state, ctx.theme.button);
	SDL_FRect rect = getScreenRect(parentOffset);

	ctx.renderer->drawRectShape(rect, fillCol, outlineCol);

	if (content) {
		if (const glm::vec2 size = content->getSize(); size == glm::vec2{0, 0}) {
			LayoutEngine::LayoutItem textItem;
			textItem.idealSize = content->getIdealContentSize(ctx);
			textItem.hAlign = LayoutEngine::Alignment::Center;
			textItem.vAlign = LayoutEngine::Alignment::Center;

			const SDL_FRect localSlot = {0.0f, 0.0f, rect.w, rect.h};

			std::array solveItems = {textItem};
			LayoutEngine::solveStack(solveItems, localSlot);

			content->setBounds(solveItems[0].calculatedBounds);
		}
		content->draw(ctx, {rect.x, rect.y});
	}
}

glm::vec2 Button::getIdealContentSize(const UIContext &ctx) const
{
	const float xPad = ctx.theme.button.widthPadding * 2.0f;
	const float yPad = ctx.theme.button.heightPadding * 2.0f;

	const glm::vec2 contentSize = content->getIdealContentSize(ctx);
	return {contentSize.x + xPad, contentSize.y + yPad};
}

} // namespace Ui
