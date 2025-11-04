#pragma once
#include "Engine/ECS/Systems/InputSystem.hpp"
#include "Engine/ECS/Systems/MapActivationSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Utils/LevelHistory.hpp"
namespace GameEngine {

class PlayState final : public GameState {
	// Inherited via GameState
  public:
	PlayState(InputManager &input, TextureManager &textureManager, Renderer &renderer)
	    : inputManager(input), textureManager(textureManager), renderer(renderer)
	{
	}
	void Init(GameEngine &game) override;
	void Cleanup() override;
	void Pause() override;
	void Resume() override;
	void HandleEvents(GameEngine &game) override;
	void Update(GameEngine &game, float deltaTime) override;
	void Render(GameEngine &game, float alpha) override;

  private:
	LevelHistory history;
	InputManager &inputManager;
	TextureManager &textureManager;
	Renderer &renderer;
	ECS::Systems::RenderSystem renderSystem{renderer};
	ECS::Systems::InputSystem inputSystem{inputManager};
	ECS::Systems::MapActivationSystem mapActivationSystem;
	bool paused = false;
	float accumulator = 0.0f;
	static constexpr float stepSize = 1.0f / 240.0f;
};
} // namespace GameEngine