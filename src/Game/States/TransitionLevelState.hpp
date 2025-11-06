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
	void handleEvents(GameEngine &game) override;
	void update(GameEngine &game, const float deltaTime) override;
	void render(GameEngine &game, const float alpha) override;

  private:
	float progress = 0.0f; // 0 = start, 1 = full black
	float duration = 0.5f;
	bool loadingDone = false;
};
} // namespace GameEngine