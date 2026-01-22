
#include "GuiManager.hpp"

#include <ranges>

#include "Engine/Graphics/Renderer.hpp"

namespace Ui {
void GuiManager::setRoot(std::unique_ptr<Widget> root)
{
	rootWidget = std::move(root);
}

void GuiManager::updateLayout() const
{
	if (!rootWidget)
		return;
	const auto [screenWidth, screenHeight] = context->renderer->getLogicalResolution();
	const BoxConstraints screenConstraints = BoxConstraints::tight(screenWidth, screenHeight);

	rootWidget->measure(*context, screenConstraints);

	// 2. Layout Pass: Set the root to its preferred size (or fullscreen)
	const SDL_FRect finalRect = {0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight)};

	rootWidget->layout(*context, finalRect);
}

void GuiManager::handleEvent(const SDL_Event &event) const
{
	if (event.type != SDL_EVENT_MOUSE_MOTION && event.type != SDL_EVENT_MOUSE_BUTTON_DOWN
	    && event.type != SDL_EVENT_MOUSE_BUTTON_UP) {
		return;
	}

	float mx = 0, my = 0;

	// Extract raw Window coordinates based on event type
	if (event.type == SDL_EVENT_MOUSE_MOTION) {
		mx = event.motion.x;
		my = event.motion.y;
	} else {
		mx = event.button.x;
		my = event.button.y;
	}

	// CONVERT to Logical Coordinates
	const glm::vec2 mouseLogicalPos = context->renderer->windowToLogical(mx, my);

	if (rootWidget)
		rootWidget->handleEvent(mouseLogicalPos, event);
}

void GuiManager::render() const
{
	if (rootWidget)
		rootWidget->draw(*context, {0, 0});
}
} // namespace Ui