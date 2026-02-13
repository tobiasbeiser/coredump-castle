
#include "GameFinishedState.hpp"

#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/BlinkSystem.hpp"
#include "Engine/ECS/Systems/ChargeTelegraphSystem.hpp"
#include "Engine/ECS/Systems/MovementSystem.hpp"
#include "Engine/ECS/Systems/ReviveSystem.hpp"
#include "Engine/ECS/Systems/StateMachineSystem.hpp"
#include "Engine/ECS/Systems/StateSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Graphics/Gui/UiViewFactory.hpp"
#include "Engine/Graphics/Gui/Utils.hpp"
#include "MainMenuState.hpp"
#include "PlayState.hpp"

namespace GameEngine {

void GameFinishedState::init(GameEngine &game)
{
	rootWidget = UiViewFactory::createCongratulationsScreen(game.getUiContext(), game);
}

void GameFinishedState::cleanup() {}

void GameFinishedState::pause()
{
	paused = true;
}

void GameFinishedState::resume()
{
	paused = false;
}

void GameFinishedState::handleEvents(GameEngine &game, const SDL_Event &event) {}

void GameFinishedState::update(GameEngine &game, const float deltaTime)
{
	if (paused) {
		return;
	}
	inputSystem.update(game.getRegistry());

	ECS::Systems::AnimationSystem::update(game.getRegistry(), deltaTime);

	if (inputManager.isKeyDown(Key::Up)) {
		game.getRegistry().clearAll();
		game.getSaveSystem().deleteCurrentSave();
		game.resetToState(
		    std::make_unique<MainMenuState>(game.getInputManager(), game.getTextureManager(), game.getRenderer()));
	}
}

void GameFinishedState::render(GameEngine &game, const float alpha)
{
	rootWidget->draw(game.getUiContext(), {0, 0});
}

} // namespace GameEngine
