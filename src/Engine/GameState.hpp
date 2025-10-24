#pragma once
#include "GameEngine.hpp"
#include <memory>

// concept taken from http://gamedevgeek.com/tutorials/managing-game-states-in-c/
namespace GameEngine {
class GameState {
  public:
	virtual ~GameState() = default;
	virtual void Init(GameEngine &game) = 0;
	virtual void Cleanup() = 0;

	virtual void Pause() = 0;
	virtual void Resume() = 0;

	virtual void HandleEvents(GameEngine &game) = 0;
	virtual void Update(GameEngine &game, const float deltaTime) = 0;
	virtual void Render(GameEngine &game, const float alpha) = 0;

	void ChangeState(GameEngine &game, std::unique_ptr<GameState> state) { game.ChangeState(std::move(state)); }

  protected:
	GameState() = default;
};
} // namespace GameEngine