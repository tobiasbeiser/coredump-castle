
#include "GameOverState.hpp"

#include "MainMenuState.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Graphics/Gui/UiViewFactory.hpp"
#include "Engine/Graphics/Gui/Utils.hpp"
#include "PlayState.hpp"
#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/BlinkSystem.hpp"
#include "Engine/ECS/Systems/ChargeTelegraphSystem.hpp"
#include "Engine/ECS/Systems/MovementSystem.hpp"
#include "Engine/ECS/Systems/ReviveSystem.hpp"
#include "Engine/ECS/Systems/StateMachineSystem.hpp"
#include "Engine/ECS/Systems/StateSystem.hpp"

namespace GameEngine {

void GameOverState::init(GameEngine &game)
{
	rootWidget = UiViewFactory::createGameOverScreen(guiManager.getContext(), game);
	guiManager.updateLayout(rootWidget.get());
}

void GameOverState::cleanup() {}

void GameOverState::pause()
{
	paused = true;
}

void GameOverState::resume()
{
	paused = false;
}

void GameOverState::handleEvents(GameEngine &game, const SDL_Event &event)
{
}

void GameOverState::update(GameEngine &game, const float deltaTime)
{
	if (paused) {
		return;
	}
	inputSystem.update(game.getRegistry());

	accumulator += deltaTime;
	while (accumulator >= stepSize) {
		accumulator -= stepSize;
		ECS::Systems::StateMachineSystem::update(game.getRegistry(), stepSize);
		ECS::Systems::MovementSystem::update(game.getRegistry(), stepSize);
		ECS::Systems::PhysicsSystem::update(game.getRegistry(), stepSize);
	}

	ECS::Systems::StateSystem::update(game.getRegistry());

	ECS::Systems::AnimationSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::BlinkSystem::update(game.getRegistry());
	ECS::Systems::ChargeTelegraphSystem::update(game.getRegistry(), deltaTime);

	if (inputManager.isKeyDown(Key::Escape)) {
		game.getRegistry().clearAll();
		game.getSaveSystem().clearCurrentSaveName();
		game.resetToState(std::make_unique<MainMenuState>(game.getInputManager(), game.getTextureManager(), game.getGuiManager(), game.getRenderer()));
	} else if (inputManager.isKeyDown(Key::Up)) {
		game.popState();
		ECS::Systems::ReviveSystem::update(game.getRegistry(), game, spawners);
	}

}

void GameOverState::render(GameEngine &game, const float alpha)
{
	rootWidget->draw(guiManager.getContext(), {0, 0});
}

} // namespace GameEngine
