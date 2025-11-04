#include "PlayState.hpp"
#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/InteractionSystem.hpp"
#include "Engine/ECS/Systems/MeleeSystem.hpp"
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
	auto factory = WorldFactory(game.getRegistry(), projectPath, game.getTextureManager());

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

	accumulator += deltaTime;

	mapActivationSystem.Update(game.getRegistry());

	inputSystem.update(game.getRegistry());

	ECS::Systems::JumpSystem::Update(game.getRegistry());
	ECS::Systems::MeleeSystem::Update(game.getRegistry(), deltaTime);

	ECS::Systems::MovementSystem::Update(game.getRegistry(), deltaTime);

	while (accumulator >= stepSize) {
		accumulator -= stepSize;

		ECS::Systems::PhysicsSystem::Update(game.getRegistry(), stepSize);
		ECS::Systems::CollisionSystem::update(game.getRegistry());
	}

	ECS::Systems::StateSystem::Update(game.getRegistry());
	ECS::Systems::AnimationSystem::Update(game.getRegistry(), deltaTime);

	ECS::Systems::InteractionSystem::Update(game, game.getRegistry());
}

void PlayState::Render(GameEngine &game, const float alpha)
{
	// std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	renderSystem.Update(game.getRegistry(), alpha);
	// std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
	// DebugSystem::Debug("Render Time: " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time1 -
	// now).count()));
}
} // namespace GameEngine