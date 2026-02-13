#pragma once

#pragma once
#include "Engine/ECS/Systems/InputSystem.hpp"
#include "Engine/ECS/Systems/MapRenderSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "Game/Spawners/EnemySpawner.hpp"

namespace GameEngine {

class GameFinishedState final : public GameState {
	// Inherited via GameState
  public:
	GameFinishedState(InputManager &input, Renderer &renderer) : inputManager(input), renderer(renderer) {}
	void init(GameEngine &game) override;
	void cleanup() override;
	void pause() override;
	void resume() override;
	void handleEvents(GameEngine &game, const SDL_Event &event) override;
	void update(GameEngine &game, float deltaTime) override;
	void render(GameEngine &game, float alpha) override;

  private:
	InputManager &inputManager;
	Renderer &renderer;
	ECS::Systems::RenderSystem renderSystem{renderer};
	ECS::Systems::InputSystem inputSystem{inputManager};
	std::unique_ptr<Ui::Widget> rootWidget{};
	bool paused = false;
};
} // namespace GameEngine
