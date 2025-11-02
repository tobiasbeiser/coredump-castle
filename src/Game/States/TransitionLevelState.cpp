#include "TransitionLevelState.hpp"
#include "Game/Factories/LevelFactory.hpp"
#include "Game/States/PlayState.hpp"
#include <chrono>
#include <thread>

GameEngine::TransitionLevelState::TransitionLevelState(const std::string levelName, const std::string spawnPoint)
    : nextLevelName(levelName), spawnPoint(spawnPoint)
{
}

void GameEngine::TransitionLevelState::Init(GameEngine &game) {}

void GameEngine::TransitionLevelState::Cleanup() {}

void GameEngine::TransitionLevelState::Pause() {}

void GameEngine::TransitionLevelState::Resume() {}

void GameEngine::TransitionLevelState::HandleEvents(GameEngine &game) {}

void GameEngine::TransitionLevelState::Update(GameEngine &game, const float deltaTime)
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
			game.PopState(); // done, remove transition
		}
	}
}

void GameEngine::TransitionLevelState::Render(GameEngine &game, const float alpha)
{
	SDL_FColor black = {0, 0, 0, 255};
	game.getRenderer().RenderCircularFade(progress, black);
}
