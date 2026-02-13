#pragma once
#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/Gui/Core/UiContext.hpp"

namespace Ui {
class Image final : public Widget {
  public:
	explicit Image(SDL_Texture *tex);
	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override;

	void draw(const UIContext &ctx, glm::vec2 parentOffset) const override;

  private:
	SDL_Texture *texture;
};
} // namespace Ui