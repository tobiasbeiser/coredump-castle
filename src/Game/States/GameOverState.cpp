
#include "GameOverState.hpp"

#include "MainMenuState.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Graphics/Gui/UiViewFactory.hpp"
#include "Engine/Graphics/Gui/Utils.hpp"
#include "PlayState.hpp"
#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/MovementSystem.hpp"
#include "Engine/ECS/Systems/ReviveSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"
#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ProjectileSystem.hpp"

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
	ECS::Systems::ChaseSystem::update(game.getRegistry(), deltaTime);

	accumulator += deltaTime;
	while (accumulator >= stepSize) {
		accumulator -= stepSize;
		ECS::Systems::PatrolSystem::update(game.getRegistry());
		ECS::Systems::MovementSystem::update(game.getRegistry(), stepSize);
		ECS::Systems::PhysicsSystem::update(game.getRegistry(), stepSize);
	}

	ECS::Systems::AnimationSystem::update(game.getRegistry(), deltaTime);
	if (inputManager.isKeyDown(Key::Escape)) {
		game.getRegistry().clearAll();
		game.getSaveSystem().clearCurrentSaveName();
		game.resetToState(std::make_unique<MainMenuState>(game.getInputManager(), game.getTextureManager(), game.getGuiManager(), game.getRenderer()));
	} else if (inputManager.isKeyDown(Key::Up)) {
		game.popState();
		ECS::Systems::ReviveSystem::update(game.getRegistry(), game, deltaTime);
	}

}

void GameOverState::render(GameEngine &game, const float alpha)
{
	rootWidget->draw(guiManager.getContext(), {0, 0});
}

} // namespace GameEngine
