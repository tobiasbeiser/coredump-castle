#pragma once
#include "Engine/ECS/Systems/InputSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/TextureManager.hpp"

namespace GameEngine {

class PauseMenuState final : public GameState {
	// Inherited via GameState
  public:
	PauseMenuState(InputManager &input, TextureManager &textureManager, Renderer &renderer)
	    : inputManager(input), textureManager(textureManager), renderer(renderer)
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
	void setupUI(GameEngine &game);
	void showInventory(GameEngine &game);
	void showSettingsMenu(GameEngine &game);
	InputManager &inputManager;
	TextureManager &textureManager;
	Renderer &renderer;
	ECS::Systems::InputSystem inputSystem{inputManager};
	std::unique_ptr<Ui::Widget> rootWidget{};
	bool paused = false;
};
} // namespace GameEngine
