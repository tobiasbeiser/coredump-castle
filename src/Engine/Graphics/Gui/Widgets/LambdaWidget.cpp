
#include "LambdaWidget.hpp"

namespace Ui {

void LambdaWidget::layout(const UIContext &ctx, SDL_FRect allocatedRect)
{
	Widget::layout(ctx, allocatedRect);
}

bool LambdaWidget::handleEvent(const glm::vec2 localMousePos, const SDL_Event &event)
{
	return false;
}

void LambdaWidget::draw(const UIContext &ctx, const glm::vec2 parentOffset) const
{
	const SDL_FRect rect = getScreenRect(parentOffset);

	// RAII automatically resets the render scope after exiting
	auto scope = ctx.renderer->createRenderScope(rect, zoom);
	if (drawCallback) {
		drawCallback();
	}
}

void LambdaWidget::setContentZoom(const float newZoom)
{
	zoom = newZoom;
}

glm::vec2 LambdaWidget::performMeasure(const UIContext &ctx, BoxConstraints constraints) const
{
	return {constraints.maxWidth, constraints.maxHeight};
}

} // namespace Ui
