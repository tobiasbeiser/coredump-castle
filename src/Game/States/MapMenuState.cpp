
#include "MapMenuState.hpp"

#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/MenuStateToggleSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Graphics/Gui/UiViewFactory.hpp"
#include "Engine/Graphics/Gui/Utils.hpp"
#include "PlayState.hpp"

namespace GameEngine {

void MapMenuState::init(GameEngine &game)
{
	mapRenderSystem.initFocusPoint(game.getRegistry());
	rootWidget = UiViewFactory::createMapScreen(game.getUiContext(), mapRenderSystem.getMapSizeFactor());
}

void MapMenuState::cleanup() {}

void MapMenuState::pause()
{
	paused = true;
}

void MapMenuState::resume()
{
	paused = false;
}

void MapMenuState::handleEvents(GameEngine &game, const SDL_Event &event)
{
	if (event.type == SDL_EVENT_MOUSE_WHEEL) {
		mapRenderSystem.handleZoom(event.wheel.y);
	}
}

void MapMenuState::update(GameEngine &game, const float deltaTime)
{
	inputSystem.update(game.getRegistry());
	ECS::Systems::MenuStateToggleSystem::update(game.getRegistry(), game);
	ECS::Systems::AnimationSystem::update(game.getRegistry(), deltaTime);
}

void MapMenuState::render(GameEngine &game, const float alpha)
{
	if (SDL_Texture *backgroundTexture =
			&game.getTextureManager().getTexture("assets/tiled/tilesets/Ui/MenuBackground.png")) {
		SDL_SetTextureColorMod(backgroundTexture, 170, 170, 170);
		SDL_SetTextureAlphaMod(backgroundTexture, 255);
		renderer.renderTexture(*backgroundTexture, std::nullopt, std::nullopt, false);
			}
	rootWidget->draw(game.getUiContext(), {0, 0});
	mapRenderSystem.update(game.getRegistry(), game);
}

} // namespace GameEngine
