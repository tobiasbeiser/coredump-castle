
#include "MapMenuState.hpp"

#include "Engine/ECS/Systems/MapToggleSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Graphics/Gui/UiViewFactory.hpp"
#include "Engine/Graphics/Gui/Utils.hpp"
#include "PlayState.hpp"
#include "Engine/ECS/Systems/AnimationSystem.hpp"

namespace GameEngine {

void MapMenuState::init(GameEngine &game)
{
	mapRenderSystem.initFocusPoint(game.getRegistry());
	rootWidget = UiViewFactory::createMapScreen(guiManager.getContext(), game, mapRenderSystem.getMapSizeFactor());
	guiManager.updateLayout(rootWidget.get());
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
	ECS::Systems::MapToggleSystem::update(game.getRegistry(), game);
	ECS::Systems::AnimationSystem::update(game.getRegistry(), deltaTime);
}

void MapMenuState::render(GameEngine &game, const float alpha)
{
	rootWidget->draw(guiManager.getContext(), {0, 0});
	mapRenderSystem.update(game.getRegistry(), game);
}

} // namespace GameEngine
