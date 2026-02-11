#pragma once
#include "Engine/ECS/Systems/InputSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/GameState.hpp"
#include "Engine/ECS/Systems/MapRenderSystem.hpp"
#include "Engine/Graphics/Gui/GuiManager.hpp"
#include "Engine/Graphics/TextureManager.hpp"

namespace GameEngine {

class PauseMenuState final : public GameState {
	// Inherited via GameState
  public:
	PauseMenuState(InputManager &input, TextureManager &textureManager, Ui::GuiManager &guiManager, Renderer &renderer)
	    : inputManager(input), textureManager(textureManager), guiManager(guiManager), renderer(renderer)
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
	static void transitionToPlayState(GameEngine &game);
	InputManager &inputManager;
	TextureManager &textureManager;
	Ui::GuiManager &guiManager;
	Renderer &renderer;
	ECS::Systems::InputSystem inputSystem{inputManager};
	std::unique_ptr<Ui::Widget> rootWidget{};
	bool paused = false;
};
} // namespace GameEngine
