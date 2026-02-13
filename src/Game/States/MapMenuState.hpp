#pragma once
#include "Engine/ECS/Systems/InputSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/GameState.hpp"
#include "Engine/ECS/Systems/MapRenderSystem.hpp"
#include "Engine/Graphics/Gui/Base/Widget.hpp"
#include "Engine/Graphics/TextureManager.hpp"

namespace GameEngine {

class MapMenuState final : public GameState {
	// Inherited via GameState
  public:
	MapMenuState(InputManager &input, TextureManager &textureManager, Renderer &renderer)
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
	InputManager &inputManager;
	TextureManager &textureManager;
	Renderer &renderer;
	static constexpr float initialScalingFactor = 0.30;
	ECS::Systems::MapRenderSystem mapRenderSystem{renderer, initialScalingFactor};
	ECS::Systems::InputSystem inputSystem{inputManager};
	std::unique_ptr<Ui::Widget> rootWidget{};
	bool paused = false;
};
} // namespace GameEngine
