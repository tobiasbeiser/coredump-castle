#include "MapToggleSystem.hpp"

#include "Engine/ECS/Components/Gameplay/MapToggleComponent.hpp"
#include "Game/States/MapMenuState.hpp"

void GameEngine::ECS::Systems::MapToggleSystem::update(Registry &registry, GameEngine &game)
{
	for (auto view = registry.view<MapToggleComponent>(); const auto entity : view) {
		if (auto &[mapToggled, mapShowing] = view.get<MapToggleComponent>(entity); mapToggled) {
			if (!mapShowing) {
				mapShowing = true;
				game.pushState(std::make_unique<MapMenuState>(game.getInputManager(), game.getTextureManager(),
				                                              game.getGuiManager(), game.getRenderer()));
			} else {
				mapShowing = false;
				game.popState();
			}
		}
	}
}
