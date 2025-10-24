#include "PlayState.hpp"
#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/InteractionSystem.hpp"
#include "Engine/ECS/Systems/JumpSystem.hpp"
#include "Engine/ECS/Systems/MovementSystem.hpp"
#include "Engine/ECS/Systems/PhysicsSystem.hpp"
#include "Engine/ECS/Systems/StateSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "Game/Factories/LevelFactory.hpp"
#include <time.h>

namespace GameEngine {

void PlayState::Init(GameEngine &game)
{
	game.PushState(std::make_unique<TransitionLevelState>("Level1", "Player"));
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
	ECS::Systems::MovementSystem::Update(game.registry, deltaTime);
	ECS::Systems::PhysicsSystem::Update(game.registry, deltaTime);
	ECS::Systems::CollisionSystem::Update(game.registry);

	ECS::Systems::StateSystem::Update(game.registry);
	ECS::Systems::AnimationSystem::Update(game.registry, deltaTime);

	
	ECS::Systems::LevelTransitionData transitionData{history, currentLevelName};
	ECS::Systems::InteractionSystem::Update(game, game.registry, transitionData);
}

void PlayState::Render(GameEngine &game, const float alpha)
{
	renderSystem.Update(game.registry, alpha);
}
} // namespace GameEngine