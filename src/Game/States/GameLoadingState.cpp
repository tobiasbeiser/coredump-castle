#include "GameLoadingState.hpp"

#include "Engine/Graphics/Gui/UiViewFactory.hpp"

namespace GameEngine {

GameLoadingState::GameLoadingState(StateFactory factory): nextStateFactory(std::move(factory))
{
}

void GameLoadingState::init(GameEngine &game)
{
	rootWidget = UiViewFactory::createLoadingScreen(game.getUiContext());
}

void GameLoadingState::update(GameEngine &game, float deltaTime)
{
	if (hasRenderedOnce) {
		game.changeState(nextStateFactory());
	}
}

void GameLoadingState::render(GameEngine &game, float alpha)
{
	rootWidget->draw(game.getUiContext(), {0, 0});
	hasRenderedOnce = true;
}
} // namespace GameEngine