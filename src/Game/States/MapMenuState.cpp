
#include "MapMenuState.hpp"

#include "Engine/ECS/Systems/UiMapSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Graphics/Gui/UiViewFactory.hpp"
#include "Engine/Graphics/Gui/Utils.hpp"
#include "PlayState.hpp"

namespace GameEngine {

void MapMenuState::init(GameEngine &game)
{
	std::function<void()> menuFillDraw = [this, &game] { renderSystem.updateIngameMap(game.getRegistry()); };
	rootWidget = UiViewFactory::createMapScreen(guiManager.getContext(), menuFillDraw, game);
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
	Ui::Utils::passMouseEvent(rootWidget.get(), event, renderer);
}

void MapMenuState::update(GameEngine &game, const float deltaTime)
{
	inputSystem.update(game.getRegistry());
	ECS::Systems::UiMapSystem::update(game.getRegistry(), game);
}

void MapMenuState::render(GameEngine &game, const float alpha)
{
	rootWidget->draw(guiManager.getContext(), {0, 0});
}

} // namespace GameEngine
