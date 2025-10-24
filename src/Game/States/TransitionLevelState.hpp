#pragma once
#include "Engine/GameState.hpp"
#include <string>

namespace GameEngine {
class TransitionLevelState : public GameState {
	
	public:
	TransitionLevelState(const std::string levelName, const std::string spawnPoint);

	// Inherited via GameState
	void Init(GameEngine &game) override;
	void Cleanup() override;
	void Pause() override;
	void Resume() override;
	void HandleEvents(GameEngine &game) override;
	void Update(GameEngine &game, const float deltaTime) override;
	void Render(GameEngine &game, const float alpha) override;

  private:
	const std::string nextLevelName;
	const std::string spawnPoint;
	float progress = 0.0f; // 0 = start, 1 = full black
	float duration = 0.5f;
	bool loadingDone = false;


};
} // namespace GameEngine