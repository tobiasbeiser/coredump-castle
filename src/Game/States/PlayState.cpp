#include "PlayState.hpp"
#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/InteractionSystem.hpp"
#include "Engine/ECS/Systems/JumpSystem.hpp"
#include "Engine/ECS/Systems/MovementSystem.hpp"
#include "Engine/ECS/Systems/PhysicsSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/ECS/Systems/StateSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "Game/Factories/WorldFactory.hpp"

namespace GameEngine {

void PlayState::Init(GameEngine &game)
{
	// game.PushState(std::make_unique<TransitionLevelState>("MainHub", "Player"));
	const std::filesystem::path projectPath = "assets/tiled/CoredumpCastle.tiled-project";
	auto factory = WorldFactory(game.get_registry(), projectPath, game.get_texture_manager());

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
	mapActivationSystem.Update(game.get_registry());

	inputSystem.Update(game.get_registry());

	ECS::Systems::JumpSystem::Update(game.get_registry());
	ECS::Systems::MovementSystem::Update(game.get_registry(), deltaTime);
	std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	ECS::Systems::PhysicsSystem::Update(game.get_registry(), deltaTime);
	std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
	ECS::Systems::CollisionSystem::Update(game.get_registry());
	std::chrono::high_resolution_clock::time_point time2 = std::chrono::high_resolution_clock::now();

	/*DebugSystem::Debug("Physics Time: "+ std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time1 -
	   now).count()) + "\n"+ "Collision Time: "
	                   + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time2 -
	   time1).count()));*/

	ECS::Systems::StateSystem::Update(game.get_registry());
	ECS::Systems::AnimationSystem::Update(game.get_registry(), deltaTime);

	ECS::Systems::InteractionSystem::Update(game, game.get_registry());
}

void PlayState::Render(GameEngine &game, const float alpha)
{
	// std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	renderSystem.Update(game.get_registry(), alpha);
	// std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
	// DebugSystem::Debug("Render Time: " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time1 -
	// now).count()));
}
} // namespace GameEngine