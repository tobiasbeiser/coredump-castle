#pragma once
#include <SDL3/SDL_events.h>
#include <memory>

#include "Base/Widget.hpp"
#include "Core/UiContext.hpp"

namespace Ui {
class GuiManager {
  public:
	explicit GuiManager(std::unique_ptr<UIContext> context) : context(std::move(context)) {}

	void setRoot(std::unique_ptr<Widget> root);

	void updateLayout() const;

	void handleEvent(const SDL_Event &event) const;
	void render() const;

	UIContext &getContext() const { return *context; };

  private:
	std::unique_ptr<Widget> rootWidget;
	std::unique_ptr<UIContext> context;
};
} // namespace Ui
