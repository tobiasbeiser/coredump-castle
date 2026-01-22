#pragma once
#include "Engine/GameState.hpp"
#include <string>

namespace GameEngine {
class FadeTransitionState : public GameState {

  public:
	explicit FadeTransitionState(std::function<void()> onMidpointAction);

	void init(GameEngine &game) override;
	void cleanup() override;
	void pause() override;
	void resume() override;
	void handleEvents(GameEngine &game, const SDL_Event &event) override;
	void update(GameEngine &game, float deltaTime) override;
	void render(GameEngine &game, float alpha) override;

  private:
	std::function<void()> onMidpointAction;
	bool actionPerformed = false;
	float progress = 0.0f;
	float speed = 2.0f;
};
} // namespace GameEngine