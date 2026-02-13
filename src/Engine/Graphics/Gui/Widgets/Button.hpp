#pragma once
#include <functional>
#include <glm/vec2.hpp>
#include <memory>
#include <string_view>

#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/Gui/Core/UiContext.hpp"
#include "Engine/Graphics/Gui/Core/UiTypes.hpp"

namespace Ui {
class Label;

class Button final : public Widget {
  public:
	Button(std::string_view label, TTF_Font *font, const std::function<void()> &onClick);
	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override;
	void draw(const UIContext &ctx, glm::vec2 parentOffset) const override;
	void setButtonDisabled(const bool disabled) { isDisabled = disabled; }
	[[nodiscard]] glm::vec2 getIdealContentSize(const UIContext &ctx) const override;

  private:
	std::unique_ptr<Label> content;
	std::function<void()> callback;

	bool isHovered = false;
	bool isPressed = false;
	bool isDisabled = false;
};

} // namespace Ui
