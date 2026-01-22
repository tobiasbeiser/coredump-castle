#pragma once
#include "Engine/Graphics/Gui/Base/Widget.hpp"

namespace Ui
{
	class Label : public Widget {
	public:
		Label(std::string_view labelText, TTF_Font *font);

		void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
		bool handleEvent(glm::vec2 localMousePos, const SDL_Event &) override { return false; }
		void draw(const UIContext &ctx, glm::vec2 parentOffset) const override;

		void setColor(SDL_Color newColor);
		void setText(std::string_view newText);

	private:
		glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) const override;
		void ensureTextObject(const UIContext &ctx) const;

		std::string labelText;
		mutable bool colorDirty = true;
		mutable std::unique_ptr<TTF_Text, TTFTextDestroyer> textObject;
		TTF_Font *font;
		SDL_Color color{255, 255, 255, 255}; // white default
	};
}