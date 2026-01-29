#include "GameLoadingState.hpp"

#include "Engine/Graphics/Gui/UiViewFactory.hpp"

namespace GameEngine {

void GameLoadingState::init(GameEngine &game)
{
	rootWidget = UiViewFactory::createLoadingScreen(guiManager.getContext());
	guiManager.updateLayout(rootWidget.get());
}

void GameLoadingState::update(GameEngine &game, float deltaTime)
{
	if (hasRenderedOnce) {
		game.changeState(nextStateFactory());
	}
}

void GameLoadingState::render(GameEngine &game, float alpha)
{
	rootWidget->draw(guiManager.getContext(), {0, 0});
	hasRenderedOnce = true;
}
} // namespace GameEngine