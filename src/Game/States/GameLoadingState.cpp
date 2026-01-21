#include "GameLoadingState.hpp"

namespace GameEngine {

	void GameLoadingState::update(GameEngine& game, float deltaTime) {
		if (hasRenderedOnce) {
			game.changeState(nextStateFactory());
		}
	}

	void GameLoadingState::render(GameEngine& game, float alpha) {
		// TODO: Clear screen, raw "LOADING..." text

		hasRenderedOnce = true;
	}

} // namespace GameEngine