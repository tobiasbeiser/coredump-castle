#include "MenuStateToggleSystem.hpp"

#include "Engine/ECS/Components/Gameplay/MapToggleComponent.hpp"
#include "Game/States/MapMenuState.hpp"
#include "Game/States/PauseMenuState.hpp"

void GameEngine::ECS::Systems::MenuStateToggleSystem::update(Registry &registry, GameEngine &game)
{
	for (auto view = registry.view<MapToggleComponent>(); const auto entity : view) {
		if (auto &[mapToggled, mapShowing] = view.get<MapToggleComponent>(entity); mapToggled) {
			if (!mapShowing) {
				mapShowing = true;
				game.pushState(std::make_unique<MapMenuState>(game.getInputManager(), game.getTextureManager(),
				                                              game.getRenderer()));
			} else {
				mapShowing = false;
				game.popState();
			}
		}
	}

	for (auto view = registry.view<PauseMenuToggleComponent>(); const auto entity : view) {
		if (auto &[pauseMenuToggled, pauseMenuShowing] = view.get<PauseMenuToggleComponent>(entity); pauseMenuToggled) {
			if (!pauseMenuShowing) {
				pauseMenuShowing = true;
				game.pushState(std::make_unique<PauseMenuState>(game.getInputManager(), game.getTextureManager(),
															  game.getRenderer()));
			} else {
				pauseMenuShowing = false;
				game.popState();
			}
		}
	}
}
