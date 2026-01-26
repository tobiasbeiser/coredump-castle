
#include "Frame.hpp"

namespace Ui {

void Frame::layout(const UIContext &ctx, SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);
}

bool Frame::handleEvent(const glm::vec2 localMousePos, const SDL_Event &event)
{
	return false;
}

void Frame::draw(const UIContext &ctx, const glm::vec2 parentOffset) const
{
	const SDL_FRect rect = getScreenRect(parentOffset);
	auto [fill, outline] = ctx.theme.frame;

	ctx.renderer->drawRectShape(rect, fill, outline);
	drawBackground(ctx, parentOffset);
}

glm::vec2 Frame::performMeasure(const UIContext &ctx, BoxConstraints constraints) const
{
	return {constraints.maxWidth, constraints.maxHeight};
}

} // namespace Ui
