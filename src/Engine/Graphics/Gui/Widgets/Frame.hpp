#pragma once
#include <glm/vec2.hpp>

#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/Gui/Core/UiContext.hpp"
#include "Engine/Graphics/Gui/Core/UiTypes.hpp"

namespace Ui {

class Frame final : public Widget {
  public:
	explicit Frame(SDL_Texture *backgroundTex=nullptr);
	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override;
	void draw(const UIContext &ctx, glm::vec2 parentOffset) const override;

  private:
	void drawBackground(const UIContext &ctx, glm::vec2 parentOffset) const;
	SDL_Texture *backgroundTex;
};

} // namespace Ui
