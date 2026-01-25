#include "PlayState.hpp"
#include "Engine/ECS/Systems/AnimationSystem.hpp"
#include "Engine/ECS/Systems/BlinkSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Combat/DeathSystem.hpp"
#include "Engine/ECS/Systems/Combat/HealthSystem.hpp"
#include "Engine/ECS/Systems/Combat/HitDetectionSystem.hpp"
#include "Engine/ECS/Systems/EffectSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"
#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ProjectileSystem.hpp"
#include "Engine/ECS/Systems/Hazard/SpikeSystem.hpp"
#include "Engine/ECS/Systems/InteractionSystem.hpp"
#include "Engine/ECS/Systems/JumpSystem.hpp"
#include "Engine/ECS/Systems/MovementSystem.hpp"
#include "Engine/ECS/Systems/PhysicsSystem.hpp"
#include "Engine/ECS/Systems/RenderSystem.hpp"
#include "Engine/ECS/Systems/RespawnSystem.hpp"
#include "Engine/ECS/Systems/StateSystem.hpp"
#include "Engine/ECS/Systems/TooltipSystem.hpp"
#include "Engine/ECS/Systems/VignetteSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "Game/Factories/WorldFactory.hpp"

namespace GameEngine {

void PlayState::init(GameEngine &game)
{
	const std::filesystem::path projectPath = "assets/tiled/CoredumpCastle.tiled-project";
	auto factory = WorldFactory(game.getRegistry(), projectPath, game.getTextureManager(), game.getInputManager());

	const std::filesystem::path worldPath = "assets/tiled/coredumpCastle.world";
	factory.createWorld(worldPath);
	game.getSaveSystem().loadSavedGame(game.getRegistry());

	game.getAudioManager().playMusic("assets/audio/music/main_theme.ogg", 2500);
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

void PlayState::handleEvents(GameEngine &game, const SDL_Event &event) {}

void PlayState::update(GameEngine &game, const float deltaTime)
{
	PROFILE_SCOPE("AllSystems");
	if (paused) {
		return;
	}
	mapActivationSystem.update(game.getRegistry());
	inputSystem.update(game.getRegistry());

	ECS::Systems::ChaseSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::ProjectileSystem::update(game.getRegistry(), deltaTime, game.getTextureManager());
	ECS::Systems::JumpSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::AttackSystem::update(game.getRegistry(), deltaTime, [&](const ECS::Entity e) {
		if (game.getRegistry().has<PlayerTag>(e)) {
			effectFactory.createEffectOnEntity(e, EffectType::PlayerSwoosh);
		}
	});
	ECS::Systems::HitDetectionSystem::update(game.getRegistry(),
	                                         [&](const ECS::Entity attacker, const ECS::Entity victim) {
		                                         effectFactory.createIsHitEffect(attacker, victim);
	                                         });

	accumulator += deltaTime;
	while (accumulator >= stepSize) {
		accumulator -= stepSize;
		ECS::Systems::PatrolSystem::update(game.getRegistry());
		ECS::Systems::MovementSystem::update(game.getRegistry(), stepSize);
		ECS::Systems::PhysicsSystem::update(game.getRegistry(), stepSize);
	}

	ECS::Systems::SpikeSystem::update(game.getRegistry());
	ECS::Systems::HealthSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::StateSystem::update(game.getRegistry());

	ECS::Systems::AnimationSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::BlinkSystem::update(game.getRegistry());
	ECS::Systems::EffectSystem::update(game.getRegistry(), deltaTime);
	ECS::Systems::VignetteSystem::update(game.getRegistry(), deltaTime);

	ECS::Systems::DeathSystem::update(game.getRegistry(), deltaTime);

	ECS::Systems::InteractionSystem::update(game, game.getRegistry());
	ECS::Systems::TooltipSystem::update(game.getRegistry());
	ECS::Systems::RespawnSystem::update(game.getRegistry(), game, deltaTime);
}

void PlayState::render(GameEngine &game, const float alpha)
{
	renderSystem.update(game.getRegistry(), alpha);
}
} // namespace GameEngine