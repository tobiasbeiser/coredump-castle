#pragma once
#include "Container.hpp"

namespace Ui {
class ScrollContainer : public Container {
  public:
	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override;
	void draw(const UIContext &context, glm::vec2 parentOffset) const override;

  private:
	float scrollY = 0.0f;
};
} // namespace Ui