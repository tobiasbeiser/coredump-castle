#pragma once
#include "Engine/Graphics/Gui/Base/Widget.hpp"

namespace Ui {
class Label : public Widget {
  public:
	Label(std::string_view labelText, TTF_Font *font, SDL_Color color = {255, 255, 255, 255});

	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &) override { return false; }
	void draw(const UIContext &ctx, glm::vec2 parentOffset) const override;
	glm::vec2 getIdealContentSize(const UIContext &ctx) const override;

	void setColor(SDL_Color newColor);

  private:
	void ensureTextObject(const UIContext &ctx) const;

	std::string labelText;
	mutable bool colorDirty = true;
	mutable std::unique_ptr<TTF_Text, TTFTextDestroyer> textObject;
	TTF_Font *font;
	SDL_Color color;
};

} // namespace Ui