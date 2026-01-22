#include "GameLoadingState.hpp"

#include "Engine/Graphics/Gui/UiViewFactory.hpp"

namespace GameEngine {

void GameLoadingState::init(GameEngine &game)
{
	guiManager.setRoot(UiViewFactory::createLoadingScreen(guiManager.getContext()));
	guiManager.updateLayout();
}

void GameLoadingState::update(GameEngine &game, float deltaTime)
{
	if (hasRenderedOnce) {
		game.changeState(nextStateFactory());
	}
}

void GameLoadingState::render(GameEngine &game, float alpha)
{
	guiManager.render();
	hasRenderedOnce = true;
}
} // namespace GameEngine