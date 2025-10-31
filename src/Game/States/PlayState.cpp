#include "PlayState.hpp"
#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/InteractionSystem.hpp"
#include "Engine/ECS/Systems/MeleeSystem.hpp"
#include "Engine/ECS/Systems/JumpSystem.hpp"
#include "Engine/ECS/Systems/MovementSystem.hpp"
#include "Engine/ECS/Systems/PhysicsSystem.hpp"
#include "Engine/ECS/Systems/StateSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "Game/Factories/WorldFactory.hpp"


namespace GameEngine {

void PlayState::Init(GameEngine &game)
{
	// game.PushState(std::make_unique<TransitionLevelState>("MainHub", "Player"));
	const std::filesystem::path projectPath = "assets/tiled/CoredumpCastle.tiled-project";
	auto factory = WorldFactory(game.registry, projectPath, game.textureManager);

	const std::filesystem::path worldPath = "assets/tiled/coredumpCastle.world";
	factory.CreateWorld(worldPath);
}

void PlayState::Cleanup() {}

void PlayState::Pause()
{
	paused = true;
}

void PlayState::Resume()
{
	paused = false;
}

void PlayState::HandleEvents(GameEngine &game) {}

void PlayState::Update(GameEngine &game, const float deltaTime)
{
	if (paused) {
		return;
	}
	inputSystem.Update(game.registry);

	ECS::Systems::JumpSystem::Update(game.registry);
	ECS::Systems::MeleeSystem::Update(game.registry, deltaTime);

	ECS::Systems::MovementSystem::Update(game.registry, deltaTime);
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	ECS::Systems::PhysicsSystem::Update(game.registry, deltaTime);
	std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
	ECS::Systems::CollisionSystem::Update(game.registry);
	std::chrono::high_resolution_clock::time_point time2 = std::chrono::high_resolution_clock::now();

	/*DebugSystem::Debug("Physics Time: "+ std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time1 - now).count()) + "\n"+
		"Collision Time: "
	                   + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time2 - time1).count()));*/

	ECS::Systems::StateSystem::Update(game.registry);
	ECS::Systems::AnimationSystem::Update(game.registry, deltaTime);

	ECS::Systems::LevelTransitionData transitionData{history, currentLevelName};
	ECS::Systems::InteractionSystem::Update(game, game.registry, transitionData);
}

void PlayState::Render(GameEngine &game, const float alpha)
{
	//std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	renderSystem.Update(game.registry, alpha);
	//std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
	//DebugSystem::Debug("Render Time: " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time1 - now).count()));
}
} // namespace GameEngine