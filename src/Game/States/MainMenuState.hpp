#pragma once
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/GameState.hpp"
#include "Engine/Graphics/FontManager.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "Engine/Graphics/Gui/GuiManager.hpp"


namespace GameEngine {

class MainMenuState final : public GameState {
	// Inherited via GameState
  public:
	MainMenuState(InputManager &input, TextureManager &textureManager, FontManager &fontManager, Renderer &renderer)
	    : inputManager(input), textureManager(textureManager), fontManager(fontManager), renderer(renderer)
	{
	}
	void init(GameEngine &game) override;
	void cleanup() override;
	void pause() override;
	void resume() override;
	void handleEvents(GameEngine& game, const SDL_Event& event) override;
	void update(GameEngine &game, float deltaTime) override;
	void render(GameEngine &game, float alpha) override;

private:
	std::unique_ptr<GuiManager> guiManager; // The Manager owns the UI:
	TTF_Font* titleFont = nullptr;
	std::unique_ptr<TTF_Text, TTFTextDestroyer> titleText;
	SDL_FRect titleRect{};
	InputManager &inputManager;
	TextureManager &textureManager;
	FontManager &fontManager;
	Renderer &renderer;
	ECS::Systems::RenderSystem renderSystem{renderer};
	bool paused = false;
	float accumulator = 0.0f;
	int counter = 0;
	static constexpr float stepSize = 1.0f / 120.0f;
};
} // namespace GameEngine