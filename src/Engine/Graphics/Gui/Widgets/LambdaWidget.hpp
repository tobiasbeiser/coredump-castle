
#pragma once
#include <functional>
#include <glm/vec2.hpp>

#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/Gui/Core/UiContext.hpp"
#include "Engine/Graphics/Gui/Core/UiTypes.hpp"

namespace Ui {

class LambdaWidget final : public Widget {
  public:
	explicit LambdaWidget(const std::function<void()> &drawCallback) : drawCallback(drawCallback) {}
	void layout(const UIContext &ctx, SDL_FRect allocatedRect) override;
	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override;
	void draw(const UIContext &ctx, glm::vec2 parentOffset) const override;
	void setContentZoom(float newZoom);

  private:
	glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) const override;
	float zoom{};
	std::function<void()> drawCallback;
};

} // namespace Ui
