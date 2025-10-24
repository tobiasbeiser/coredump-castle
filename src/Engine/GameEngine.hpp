#pragma once
#include "Graphics/Renderer.hpp"
#include "Graphics/TextureManager.hpp"
#include "Input/InputManager.hpp"
#include <SDL3/SDL.h>
#include <memory>
#include <string>
#include <vector>
#include "ECS/Registry.hpp"

namespace GameEngine {
class GameState;

class GameEngine {
  public:
	GameEngine(const std::string &title, Resolution resolution);
	~GameEngine();
	// delete copy, move and assignments as we want only one instance of the game engine
	GameEngine(const GameEngine &) = delete;
	GameEngine &operator=(const GameEngine &) = delete;
	GameEngine(GameEngine &&) = delete;
	GameEngine &operator=(GameEngine &&) = delete;
	bool Init();
	void Cleanup() noexcept;

	void ChangeState(std::unique_ptr<GameState> state);
	void PushState(std::unique_ptr<GameState> state);
	void PopState();

	void HandleEvents();
	void Update(const float deltaTime);
	void Render(const float alpha);

	bool Running() const { return running; }
	void Quit() { running = false; }

	Renderer &GetRenderer() { return renderer; }
	ECS::Registry registry;
	InputManager inputManager;
	TextureManager textureManager;
	Renderer renderer;
	std::unique_ptr<GameState> pendingPush;

  private:
	std::vector<std::unique_ptr<GameState>> states;
	bool running = false;
	const std::string title;

};
} // namespace GameEngine