#pragma once
#include "Engine/Graphics/Gui/Base/Widget.hpp"

namespace Ui {
class Spacer : public Widget {
  public:
	explicit Spacer(const float width = 0.f, const float height = 0.f) : width(width), height(height) {}

	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &) override { return false; }

	void draw(const UIContext &, glm::vec2) const override {} // Invisible

  protected:
	glm::vec2 performMeasure(const UIContext &ctx, BoxConstraints constraints) const override
	{
		return {width, height};
	}

  private:
	float width;
	float height;
};

} // namespace Ui
