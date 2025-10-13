#pragma once
#include <memory>
#include "GameEngine.hpp"


// concept taken from http://gamedevgeek.com/tutorials/managing-game-states-in-c/
class GameState {
public:
	virtual ~GameState() = default;
	virtual void Init() = 0;
	virtual void Cleanup() = 0;

	virtual void Pause() = 0;
	virtual void Resume() = 0;

	virtual void HandleEvents(GameEngine& game) = 0;
	virtual void Update(GameEngine& game, float deltaTime, const float stepSize) = 0;
	virtual void Render(GameEngine& game) = 0;

	void ChangeState(GameEngine& game, std::unique_ptr<GameState> state) {
		game.ChangeState(std::move(state));
	}

protected: 
	GameState() = default;
};

