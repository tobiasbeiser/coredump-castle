#include "Label.hpp"

namespace Ui {

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

} // namespace Ui