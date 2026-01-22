#pragma once
#include "Engine/ECS/Systems/InputSystem.hpp"
#include "Engine/ECS/Systems/MapActivationSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/GameState.hpp"
#include "Game/Factories/EffectFactory.hpp"
namespace GameEngine {

class PlayState final : public GameState {
	// Inherited via GameState
  public:
	PlayState(InputManager &input, TextureManager &textureManager, Renderer &renderer, EffectFactory &effectFactory)
	    : inputManager(input), textureManager(textureManager), renderer(renderer), effectFactory(effectFactory)
	{
	}
	void init(GameEngine &game) override;
	void cleanup() override;
	void pause() override;
	void resume() override;
	void handleEvents(GameEngine &game, const SDL_Event &event) override;
	void update(GameEngine &game, float deltaTime) override;
	void render(GameEngine &game, float alpha) override;

  private:
	InputManager &inputManager;
	TextureManager &textureManager;
	Renderer &renderer;
	ECS::Systems::RenderSystem renderSystem{renderer};
	ECS::Systems::InputSystem inputSystem{inputManager};
	ECS::Systems::MapActivationSystem mapActivationSystem{};
	EffectFactory &effectFactory;
	bool paused = false;
	float accumulator = 0.0f;
	static constexpr float stepSize = 1.0f / 120.0f;
};
} // namespace GameEngine