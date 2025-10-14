#include "PlayState.hpp"
#include "GameEngine.hpp"
#include "../Entities/Player.hpp"
#include "../Game/Level/Level.hpp"


void PlayState::Init()
{
	std::unique_ptr<Player> player = std::make_unique<Player>(inputManager, textureManager, renderer);
	this->player = player.get();
	
	std::unique_ptr<Level> level = std::make_unique<Level>(textureManager, renderer);
	this->level = level.get();

	level->Load("assets/maps/Level1.json", *player);


	gameObjects.emplace_back(std::move(level));
	gameObjects.emplace_back(std::move(player));
}

void PlayState::Cleanup()
{

}

void PlayState::Pause()
{
}

void PlayState::Resume()
{
}

void PlayState::HandleEvents(GameEngine& game)
{

}

void PlayState::Update(GameEngine& game, const float deltaTime, const float stepSize)
{
	// 1. Update all entities and apply physics
	for (auto& entity : gameObjects)
	{
		entity->Update(deltaTime, stepSize);
		physicsManager.ApplyPhysics(*entity, stepSize);
	}
	collisionSystem.Resolve(gameObjects, deltaTime);

}

void PlayState::Render(GameEngine& game)
{
	game.GetRenderer().StartFrame(25, 25, 25, 255);
	
	game.GetRenderer().CenterCameraOn(*player);


	for (auto& entity : gameObjects)
	{
		entity->Render();
	}
	level->RenderForeground();


	game.GetRenderer().EndFrame();
}
