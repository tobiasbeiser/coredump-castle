#pragma once
#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <memory>
#include "Graphics/Renderer.hpp"
#include "Graphics/TextureManager.hpp"
#include "Input/InputManager.hpp"
#include "Physics/PhysicsManager.hpp"
#include "Physics/CollisionSystem.hpp"

class GameState;

class GameEngine {
public:
	GameEngine(const std::string& title, int width, int height);
	~GameEngine();
	// delete copy, move and assignments as we want only one instance of the game engine
	GameEngine(const GameEngine&) = delete;
	GameEngine& operator=(const GameEngine&) = delete;
	GameEngine(GameEngine&&) = delete;
	GameEngine& operator=(GameEngine&&) = delete;
	bool Init();
	void Cleanup() noexcept;

	void ChangeState(std::unique_ptr<GameState> state);
	void PushState(std::unique_ptr<GameState> state);
	void PopState();


	void HandleEvents();
	void Update(const float& deltaTime, const float stepSize);
	void Render();

	bool Running() const { return running; }
	void Quit() { running = false; }

	Renderer& GetRenderer() { return renderer; }

private:
	std::vector<std::unique_ptr<GameState>> states;
	bool running = false;
	Renderer renderer;
	int width;
	int height;
	const std::string title;
	InputManager inputManager;
	TextureManager textureManager;
	PhysicsManager physicsManager;
	CollisionSystem collisionSystem;
};