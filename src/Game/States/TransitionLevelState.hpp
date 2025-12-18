#pragma once
#include "Engine/GameState.hpp"
#include <string>

namespace GameEngine {
class TransitionLevelState : public GameState {

  public:
	TransitionLevelState();

	// Inherited via GameState
	void init(GameEngine &game) override;
	void cleanup() override;
	void pause() override;
	void resume() override;
	void handleEvents(GameEngine &game, const SDL_Event &event) override;
	void update(GameEngine &game, float deltaTime) override;
	void render(GameEngine &game, float alpha) override;

  private:
	float progress = 0.0f; // 0 = start, 1 = full black
	float duration = 0.5f;
	bool loadingDone = false;
};
} // namespace GameEngine