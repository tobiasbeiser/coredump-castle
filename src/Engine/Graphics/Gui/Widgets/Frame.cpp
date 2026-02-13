
#include "Frame.hpp"

#include "Engine/Graphics/Gui/LayoutEngine.hpp"
#include "Engine/Utils/RenderUtils.hpp"

namespace Ui {
static constexpr float backgroundOffset = 4;

Frame::Frame(SDL_Texture *backgroundTex) : backgroundTex(backgroundTex) {}

bool Frame::handleEvent(const glm::vec2 localMousePos, const SDL_Event &event)
{
	return false;
}

void Frame::draw(const UIContext &ctx, const glm::vec2 parentOffset) const
{
	const SDL_FRect rect = getScreenRect(parentOffset);
	auto [fill, outline] = ctx.theme.frame;

	ctx.renderer->drawRectShape(rect, fill, outline);

	drawBackground(ctx, {rect.x, rect.y});
}

void Frame::drawBackground(const UIContext &ctx, const glm::vec2 parentOffset) const
{
	if (backgroundTex) {
		const auto size = getSize();
		auto sizedRect = LayoutEngine::positionCentered(size, size - backgroundOffset);
		sizedRect.x += parentOffset.x;
		sizedRect.y += parentOffset.y;
		ctx.renderer->renderAbsolutePositionTexture(*backgroundTex, std::nullopt, sizedRect, false, 0);
	}
}

} // namespace Ui
