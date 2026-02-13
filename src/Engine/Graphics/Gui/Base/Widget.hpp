#pragma once
#include "Engine/Graphics/Gui/Core/UiContext.hpp"
#include "Engine/Graphics/Gui/Core/UiTypes.hpp"

namespace Ui {

class Widget {
  public:
	Widget(const Widget &) = delete;
	Widget &operator=(const Widget &) = delete;
	Widget(Widget &&) = default;
	Widget &operator=(Widget &&) = default;
	virtual ~Widget() = default;

	void setBounds(const SDL_FRect &newBounds) { bounds = newBounds; }
	[[nodiscard]] const SDL_FRect &getBounds() const { return bounds; }

	[[nodiscard]] glm::vec2 getPos() const { return {bounds.x, bounds.y}; }
	[[nodiscard]] glm::vec2 getSize() const { return {bounds.w, bounds.h}; }

	virtual void draw(const UIContext &ctx, glm::vec2 parentOffset) const = 0;
	virtual bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) = 0;

	[[nodiscard]] glm::vec2 getRelativePosition() const { return {bounds.x, bounds.y}; }
	[[nodiscard]] SDL_FRect getScreenRect(const glm::vec2 parentOffset) const noexcept
	{
		return {parentOffset.x + bounds.x, parentOffset.y + bounds.y, bounds.w, bounds.h};
	}
	[[nodiscard]] virtual glm::vec2 getIdealContentSize(const UIContext &ctx) const { return {0.0f, 0.0f}; }

  protected:
	Widget() = default;

  private:
	SDL_FRect bounds{0.f, 0.f, 0.f, 0.f};
};

} // namespace Ui
