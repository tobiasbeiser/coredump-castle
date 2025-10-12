#pragma once
#include "GameState.hpp"
#include "../GameObject.hpp"


class PlayState :public GameState {
	// Inherited via GameState
public:
	PlayState(const InputManager& input, TextureManager& textureManager, Renderer& renderer, PhysicsManager& physicsManager, const CollisionSystem& collisionSystem) :
		inputManager(input), textureManager(textureManager), renderer(renderer), physicsManager(physicsManager), collisionSystem(collisionSystem) {}
	void Init() override;
	void Cleanup() override;
	void Pause() override;
	void Resume() override;
	void HandleEvents(GameEngine& game) override;
	void Update(GameEngine& game, const float deltaTime, const float stepSize) override;
	void Render(GameEngine& game) override;

private:
	std::vector<std::unique_ptr<GameObject>> gameObjects;
	Player* player = nullptr;
	Level* level = nullptr;
	const InputManager& inputManager;
	TextureManager& textureManager;
	Renderer& renderer;
	PhysicsManager& physicsManager;
	const CollisionSystem& collisionSystem;
	
};