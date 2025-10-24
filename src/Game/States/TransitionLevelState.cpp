#include "TransitionLevelState.hpp"
#include "Game/Factories/LevelFactory.hpp"
#include <thread>
#include <chrono>

GameEngine::TransitionLevelState::TransitionLevelState(const std::string levelName, const std::string spawnPoint)
    : nextLevelName(levelName), spawnPoint(spawnPoint)
{
}

void GameEngine::TransitionLevelState::Init(GameEngine &game) {}

void GameEngine::TransitionLevelState::Cleanup() {}

void GameEngine::TransitionLevelState::Pause() {}

void GameEngine::TransitionLevelState::Resume() {}

void GameEngine::TransitionLevelState::HandleEvents(GameEngine &game) {}

void GameEngine::TransitionLevelState::Update(GameEngine &game, const float deltaTime) {
	if (!loadingDone) {
		progress += deltaTime / duration;
		if (progress >= 1.0f) {
			progress = 1.0f;

			game.registry.clearAll();
			LevelFactory factory(game.registry, game.textureManager, nextLevelName, spawnPoint);
			factory.CreateLevel();

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

void GameEngine::TransitionLevelState::Render(GameEngine &game, const float alpha) {
	SDL_FColor black = {0, 0, 0, 255};
	game.GetRenderer().RenderCircularFade(progress, black);
}
