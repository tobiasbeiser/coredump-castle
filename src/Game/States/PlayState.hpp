#pragma once
#include "Engine/ECS/Systems/InputSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/GameState.hpp"

namespace GameEngine {

class PlayState : public GameState {
	// Inherited via GameState
  public:
	PlayState(const InputManager &input, TextureManager &textureManager, Renderer &renderer)
	    : inputManager(input), textureManager(textureManager), renderer(renderer){}
	void Init(GameEngine &game) override;
	void Cleanup() override;
	void Pause() override;
	void Resume() override;
	void HandleEvents(GameEngine &game) override;
	void Update(GameEngine &game, const float deltaTime) override;
	void Render(GameEngine &game,const float alpha) override;

  private:
	const InputManager &inputManager;
	TextureManager &textureManager;
	Renderer &renderer;
	ECS::Systems::RenderSystem renderSystem{renderer};
	ECS::Systems::InputSystem inputSystem{inputManager};
	bool paused = false;
};
} // namespace GameEngine