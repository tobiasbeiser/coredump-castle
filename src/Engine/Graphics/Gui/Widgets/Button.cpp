
#include "Button.hpp"

#include "Label.hpp"

namespace Ui
{
    static const Paint& resolvePaint(const WidgetState state, const ButtonStyle& style)
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

		const auto &[fillCol, outlineCol, textCol] = resolvePaint(state, ctx.theme.button);
		SDL_FRect rect = getScreenRect(parentOffset);

		ctx.renderer->drawRectShape(rect, fillCol, outlineCol);

		if (auto *label = dynamic_cast<Label *>(content.get())) {
			label->setColor(textCol);
		}
		if (content) {
			content->draw(ctx, {rect.x, rect.y});
		}
	}


}
