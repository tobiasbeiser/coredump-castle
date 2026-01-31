#pragma once
#include "Engine/ECS/Systems/InputSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/GameState.hpp"
#include "Engine/ECS/Systems/MapRenderSystem.hpp"
#include "Engine/Graphics/Gui/GuiManager.hpp"
#include "Engine/Graphics/TextureManager.hpp"

namespace GameEngine {

class MapMenuState final : public GameState {
	// Inherited via GameState
  public:
	MapMenuState(InputManager &input, TextureManager &textureManager, Ui::GuiManager &guiManager, Renderer &renderer)
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
	InputManager &inputManager;
	TextureManager &textureManager;
	Ui::GuiManager &guiManager;
	Renderer &renderer;
	static constexpr float initialScalingFactor = 0.30;
	ECS::Systems::MapRenderSystem mapRenderSystem{renderer, initialScalingFactor};
	ECS::Systems::InputSystem inputSystem{inputManager};
	std::unique_ptr<Ui::Widget> rootWidget{};
	bool paused = false;
};
} // namespace GameEngine
