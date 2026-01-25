#pragma once
#include "Engine/Graphics/Gui/Core/UiContext.hpp"
#include "Engine/Graphics/Gui/Core/UiTypes.hpp"

namespace Ui {

class Widget {
  public:
	Widget(const Widget &) = delete;
	Widget &operator=(const Widget &) = delete;
	Widget(Widget &&) = delete;
	Widget &operator=(Widget &&) = delete;
	virtual ~Widget() = default;

	glm::vec2 measure(const UIContext &ctx, BoxConstraints constraints) const;
	[[nodiscard]] glm::vec2 getMeasuredSize() const { return cachedSize.value_or(glm::vec2{0.0f, 0.0f}); };
	virtual void layout(const UIContext &ctx, const SDL_FRect allocatedRect) { bounds = allocatedRect; }

	virtual bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) = 0;
	virtual void draw(const UIContext &ctx, glm::vec2 parentOffset) const = 0;

	void setLayoutWeight(const float weight) { layoutWeight = weight; }
	[[nodiscard]] float getLayoutWeight() const { return layoutWeight; };

	void setSizeMode(SizeMode w, SizeMode h);
	void setFixedSize(glm::vec2 size);

	[[nodiscard]] SDL_FRect getBounds() const { return bounds; }
	[[nodiscard]] SizeMode getWidthMode() const { return widthMode; }
	[[nodiscard]] SizeMode getHeightMode() const { return heightMode; }
	[[nodiscard]] glm::vec2 getRelativePosition() const { return {bounds.x, bounds.y}; }

  protected:
	Widget() = default;
	[[nodiscard]] SDL_FRect getScreenRect(glm::vec2 parentOffset) const noexcept;
	virtual glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) const = 0;
	void invalidate() { isDirty = true; };

  private:
	SDL_FRect bounds = {0.0f, 0.0f, 0.0f, 0.0f}; // Position is relative to Parent

	float layoutWeight = 0.0f;
	SizeMode widthMode = SizeMode::WrapContent;
	SizeMode heightMode = SizeMode::WrapContent;
	glm::vec2 fixedSize = {0.0f, 0.0f};
	mutable std::optional<glm::vec2> cachedSize;
	mutable std::optional<BoxConstraints> lastConstraints;
	mutable bool isDirty = true;
};
} // namespace Ui
