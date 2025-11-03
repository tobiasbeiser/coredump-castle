#pragma once
#include "ECS/Registry.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/TextureManager.hpp"
#include "Input/InputManager.hpp"
#include <concepts>
#include <memory>
#include <string>
#include <vector>

namespace GameEngine {
class GameState;

template <typename T>
concept DerivedFromGameState = std::derived_from<T, GameState>;

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
	void Update(float deltaTime);
	void Render(float alpha);

	template <DerivedFromGameState T>
	T &getState()
	{
		for (auto &state : states) {
			if (auto casted = dynamic_cast<T *>(state.get())) {
				return *casted;
			}
		}
		throw std::runtime_error("State of requested type not found");
	}

	bool Running() const { return running; }
	void Quit() { running = false; }

	Renderer &getRenderer() { return renderer; }
	ECS::Registry &getRegistry() { return registry; }
	InputManager &getInputManager() { return inputManager; }
	TextureManager &getTextureManager() { return textureManager; }

  private:
	std::vector<std::unique_ptr<GameState>> states;
	bool running = false;
	const std::string title;
	ECS::Registry registry;
	Renderer renderer;
	InputManager inputManager;
	TextureManager textureManager;
};
} // namespace GameEngine