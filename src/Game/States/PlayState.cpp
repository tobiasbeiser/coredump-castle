#include "PlayState.hpp"
#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Combat/HealthSystem.hpp"
#include "Engine/ECS/Systems/Combat/HitDetectionSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"
#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
#include "Engine/ECS/Systems/Hazard/SpikeSystem.hpp"
#include "Engine/ECS/Systems/InteractionSystem.hpp"
#include "Engine/ECS/Systems/JumpSystem.hpp"
#include "Engine/ECS/Systems/MovementSystem.hpp"
#include "Engine/ECS/Systems/PhysicsSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/ECS/Systems/RespawnSystem.hpp"
#include "Engine/ECS/Systems/StateSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "Game/Factories/WorldFactory.hpp"

namespace GameEngine {

void PlayState::init(GameEngine &game)
{
	// game.PushState(std::make_unique<TransitionLevelState>("MainHub", "Player"));
	const std::filesystem::path projectPath = "assets/tiled/CoredumpCastle.tiled-project";
	auto factory = WorldFactory(game.getRegistry(), projectPath, game.getTextureManager());

	const std::filesystem::path worldPath = "assets/tiled/coredumpCastle.world";
	factory.createWorld(worldPath);
}

void PlayState::cleanup() {}

void PlayState::pause()
{
	paused = true;
}

void PlayState::resume()
{
	paused = false;
}

void PlayState::handleEvents(GameEngine &game) {}

void PlayState::update(GameEngine &game, const float deltaTime)
{
	if (paused) {
		return;
	}

	mapActivationSystem.update(game.getRegistry());

	inputSystem.update(game.getRegistry());

	ECS::Systems::PatrolSystem::update(game.getRegistry());
	ECS::Systems::ChaseSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::HitDetectionSystem::update(game.getRegistry());

	ECS::Systems::JumpSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::MovementSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::AttackSystem::update(game.getRegistry(), deltaTime);

	accumulator += deltaTime;
	while (accumulator >= stepSize) {
		accumulator -= stepSize;
		ECS::Systems::PhysicsSystem::update(game.getRegistry(), stepSize);
	}

	ECS::Systems::SpikeSystem::update(game.getRegistry());
	ECS::Systems::HealthSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::StateSystem::update(game.getRegistry());
	ECS::Systems::AnimationSystem::update(game.getRegistry(), deltaTime);

	ECS::Systems::InteractionSystem::update(game, game.getRegistry());
	ECS::Systems::RespawnSystem::update(game.getRegistry(), game, deltaTime);
}

void PlayState::render(GameEngine &game, const float alpha)
{
	// std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
	renderSystem.update(game.getRegistry(), alpha);
	// std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();
	// DebugSystem::Debug("Render Time: " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time1 -
	// now).count()));
}
} // namespace GameEngine