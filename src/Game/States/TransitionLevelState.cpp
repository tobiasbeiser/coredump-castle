#include "TransitionLevelState.hpp"
#include "Game/Factories/LevelFactory.hpp"
#include "Game/States/PlayState.hpp"
#include <chrono>
#include <thread>

GameEngine::TransitionLevelState::TransitionLevelState()
{
}

void GameEngine::TransitionLevelState::init(GameEngine &game) {}

void GameEngine::TransitionLevelState::cleanup() {}

void GameEngine::TransitionLevelState::pause() {}

void GameEngine::TransitionLevelState::resume() {}

void GameEngine::TransitionLevelState::handleEvents(GameEngine &game) {}

void GameEngine::TransitionLevelState::update(GameEngine &game, const float deltaTime)
{
	if (!loadingDone) {
		// if no level is loaded start with black screen
		if (game.getRegistry().size() == 0) {
			progress = 1.0f;
		} else {
			progress += deltaTime / duration;
		}
		if (progress >= 1.0f) {
			progress = 1.0f;


			loadingDone = true;
		}
	} else {
		// Optional: fade back in
		progress -= deltaTime / duration;
		if (progress <= 0.0f) {
			game.popState(); // done, remove transition
		}
	}
}

void GameEngine::TransitionLevelState::render(GameEngine &game, const float alpha)
{
	SDL_FColor black = {0, 0, 0, 255};
	game.getRenderer().renderCircularFade(progress, black);
}
