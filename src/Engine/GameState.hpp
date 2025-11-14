#pragma once
#include "GameEngine.hpp"
#include <memory>

// concept taken from http://gamedevgeek.com/tutorials/managing-game-states-in-c/
namespace GameEngine {
class GameState {
  public:
	GameState(const GameState &) = delete;
	GameState &operator=(const GameState &) = delete;
	GameState(GameState &&) = delete;
	GameState &operator=(GameState &&) = delete;
	virtual ~GameState() = default;

	virtual void init(GameEngine &game) = 0;
	virtual void cleanup() = 0;

	virtual void pause() = 0;
	virtual void resume() = 0;

	virtual void handleEvents(GameEngine &game) = 0;
	virtual void update(GameEngine &game, const float deltaTime) = 0;
	virtual void render(GameEngine &game, const float alpha) = 0;

	void changeState(GameEngine &game, std::unique_ptr<GameState> state) { game.changeState(std::move(state)); }

  protected:
	GameState() = default;
};
} // namespace GameEngine