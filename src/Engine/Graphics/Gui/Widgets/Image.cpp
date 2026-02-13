#include "Engine/Graphics/Gui/Widgets/Image.hpp"

Ui::Image::Image(SDL_Texture *tex) : texture(tex) {}

bool Ui::Image::handleEvent(glm::vec2 localMousePos, const SDL_Event &event)
{
	return false;
}

void Ui::Image::draw(const UIContext &ctx, glm::vec2 parentOffset) const
{
	if (texture) {
		const SDL_FRect rect = getScreenRect(parentOffset);
		ctx.renderer->renderAbsolutePositionTexture(*texture, std::nullopt, rect, false, 0);
	}
}