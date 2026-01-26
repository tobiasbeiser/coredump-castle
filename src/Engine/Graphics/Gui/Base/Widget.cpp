#include "Widget.hpp"

#include "Engine/Utils/RenderUtils.hpp"

using namespace  GameEngine::Utils::RenderUtils;

namespace Ui {
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

void Widget::setBackground(SDL_Texture *background)
{
	backgroundTex = background;
}

SDL_FRect Widget::getScreenRect(const glm::vec2 parentOffset) const noexcept
{
	return {parentOffset.x + bounds.x, parentOffset.y + bounds.y, bounds.w, bounds.h};
}

void Widget::drawBackground(const UIContext &ctx, const glm::vec2 parentOffset) const
{
	// TODO: figure out why it does not show
	if (backgroundTex) {
		ctx.renderer->renderTexture(*backgroundTex, std::nullopt, getScreenRect(parentOffset), false);
	}
}
} // namespace Ui