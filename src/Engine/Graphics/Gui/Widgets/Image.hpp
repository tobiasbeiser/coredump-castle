#pragma once
#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/Gui/Core/UiContext.hpp"

namespace Ui {
class Image final : public Widget {
  public:
	explicit Image(SDL_Texture *tex) : texture(tex) {}
	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override { return false; }

	void draw(const UIContext &ctx, glm::vec2 parentOffset) const override
	{
		if (texture) {
			const SDL_FRect rect = getScreenRect(parentOffset);
			ctx.renderer->renderAbsolutePositionTexture(*texture, std::nullopt, rect, false, 0);
		}
	}

  private:
	SDL_Texture *texture;
};
} // namespace Ui