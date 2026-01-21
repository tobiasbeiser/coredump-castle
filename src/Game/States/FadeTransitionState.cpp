#include "FadeTransitionState.hpp"
#include "Game/Factories/LevelFactory.hpp"
#include "Game/States/PlayState.hpp"

GameEngine::FadeTransitionState::FadeTransitionState(std::function<void()> onMidpointAction)
    : onMidpointAction(std::move(onMidpointAction))
{
}
void GameEngine::FadeTransitionState::init(GameEngine &game) {}

void GameEngine::FadeTransitionState::cleanup() {}

void GameEngine::FadeTransitionState::pause() {}

void GameEngine::FadeTransitionState::resume() {}

void GameEngine::FadeTransitionState::handleEvents(GameEngine &game, const SDL_Event& event) {}

void GameEngine::FadeTransitionState::update(GameEngine &game, const float deltaTime)
{
	if (!actionPerformed) {
		progress += deltaTime * speed;
		if (progress >= 1.0f) {
			progress = 1.0f;

			if (onMidpointAction) {
				onMidpointAction();
			}
			actionPerformed = true;
		}
	} else {
		progress -= deltaTime * speed;
		if (progress <= 0.0f) {
			game.popState();
		}
	}
}

void GameEngine::FadeTransitionState::render(GameEngine &game, const float alpha)
{
	constexpr SDL_FColor black = {0, 0, 0, 255};
	game.getRenderer().renderCircularFade(progress, black);
}
