#pragma once
#include "Engine/Graphics/Gui/Base/Widget.hpp"

namespace Ui {
class Container : public Widget {
  public:
	bool handleEvent(glm::vec2 localMousePos, const SDL_Event &event) override;
	void draw(const UIContext &context, glm::vec2 parentOffset) const override;
	void adoptChildren(std::vector<std::unique_ptr<Widget>> newChildren);

  protected:
	std::vector<std::unique_ptr<Widget>> children;
};
} // namespace Ui
