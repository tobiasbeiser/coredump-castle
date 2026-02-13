
#include "Frame.hpp"

#include "Engine/Graphics/Gui/LayoutEngine.hpp"
#include "Engine/Utils/RenderUtils.hpp"

namespace Ui {

bool Frame::handleEvent(const glm::vec2 localMousePos, const SDL_Event &event)
{
	return false;
}

void Frame::draw(const UIContext &ctx, const glm::vec2 parentOffset) const
{
	const SDL_FRect rect = getScreenRect(parentOffset);
	auto [fill, outline] = ctx.theme.frame;

	ctx.renderer->drawRectShape(rect, fill, outline);
}

} // namespace Ui
