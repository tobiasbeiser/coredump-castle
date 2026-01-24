
#include "GuiManager.hpp"

#include <ranges>

#include "Engine/Graphics/Renderer.hpp"

namespace Ui {

void GuiManager::updateLayout(Widget *rootWidget) const
{
	if (!rootWidget)
		return;
	const auto [screenWidth, screenHeight] = context->renderer->getLogicalResolution();
	const BoxConstraints screenConstraints = BoxConstraints::tight(screenWidth, screenHeight);

	rootWidget->measure(*context, screenConstraints);

	const SDL_FRect finalRect = {0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight)};

	rootWidget->layout(*context, finalRect);
}

} // namespace Ui