#include "Label.hpp"

namespace Ui {

Label::Label(const std::string_view labelText, TTF_Font *font, const SDL_Color color)
    : labelText(labelText), font(font), color(color)
{
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

	auto rect = getScreenRect(parentOffset);
	SDL_SetRenderDrawColor(ctx.renderer->getNativeRenderer(), 255, 0, 0, 0);
	SDL_RenderRect(ctx.renderer->getNativeRenderer(), &rect);

	const auto absolutePosition = parentOffset + getRelativePosition();
	ctx.renderer->drawText(textObject.get(), absolutePosition.x, absolutePosition.y);
}

glm::vec2 Label::getIdealContentSize(const UIContext &ctx) const
{
	if (labelText.empty())
		return {0, 0};

	ensureTextObject(ctx);

	int w = 0, h = 0;

	TTF_GetTextSize(textObject.get(), &w, &h);
	return {static_cast<float>(w), static_cast<float>(h)};
}

void Label::setColor(const SDL_Color newColor)
{
	if (newColor == color)
		return;

	color = newColor;
	colorDirty = true;
}

void Label::ensureTextObject(const UIContext &ctx) const
{
	if (!textObject) {
		textObject = ctx.renderer->createText(font, labelText);
		colorDirty = true;
	}
}

} // namespace Ui