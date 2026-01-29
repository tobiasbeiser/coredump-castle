#pragma once
#include <functional>
#include <glm/vec2.hpp>
#include <memory>
#include <string_view>

#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/Gui/Core/UiContext.hpp"
#include "Engine/Graphics/Gui/Core/UiTypes.hpp"

namespace Ui {

class Frame final : public Widget {
  public:
	Frame() {}
	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override;
	void draw(const UIContext &ctx, glm::vec2 parentOffset) const override;

  private:
	glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) const override;
};

} // namespace Ui
