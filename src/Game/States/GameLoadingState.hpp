#pragma once
#include <functional>
#include <memory>

#include "Engine/GameState.hpp"

namespace Ui {
class Widget;
}

namespace GameEngine {
class GameLoadingState : public GameState {
  public:
	using StateFactory = std::function<std::unique_ptr<GameState>()>;

	explicit GameLoadingState(StateFactory factory);

	void init(GameEngine &game) override;

	void cleanup() override {}

	void pause() override {}

	void resume() override {}

	void handleEvents(GameEngine &game, const SDL_Event &event) override {}

	void update(GameEngine &game, float deltaTime) override;
	void render(GameEngine &game, float alpha) override;

  private:
	StateFactory nextStateFactory;
	std::unique_ptr<Ui::Widget> rootWidget{};
	bool hasRenderedOnce = false;
};
} // namespace GameEngine