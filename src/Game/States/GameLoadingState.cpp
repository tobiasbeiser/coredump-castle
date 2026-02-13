#include "GameLoadingState.hpp"

#include "Engine/Graphics/Gui/UiViewFactory.hpp"

namespace GameEngine {

GameLoadingState::GameLoadingState(StateFactory factory) : nextStateFactory(std::move(factory)) {}

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
	if (SDL_Texture *backgroundTexture = &game.getTextureManager().getTexture("assets/ui/MainMenuBackground.png")) {
		SDL_SetTextureColorMod(backgroundTexture, 170, 170, 170);
		SDL_SetTextureAlphaMod(backgroundTexture, 255);
		game.getRenderer().renderTexture(*backgroundTexture, std::nullopt, std::nullopt, false);
	}
	rootWidget->draw(game.getUiContext(), {0, 0});
	hasRenderedOnce = true;
}
} // namespace GameEngine