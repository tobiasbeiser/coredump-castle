#include "RespawnSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/PendingRespawnComponent.hpp"
#include "Engine/ECS/View.hpp"
#include "Game/States/FadeTransitionState.hpp"

void GameEngine::ECS::Systems::RespawnSystem::update(Registry &registry, GameEngine &game)
{
	for (auto view =
	         registry
	             .view<PendingRespawnComponent, HealthComponent, PositionComponent, LastGroundedPositionComponent>();
	     const auto entity : view) {
		auto &healthComp = registry.get<HealthComponent>(entity);
		auto &invulnerabilityTimer = healthComp.invulnerabilityTimer;
		auto &[velocity] = registry.get<VelocityComponent>(entity);
		auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
		const auto &[lastGroundedPosition] = registry.get<LastGroundedPositionComponent>(entity);

		auto respawnLogic = [&position, &lastGroundedPosition, &velocity, &invulnerabilityTimer]() {
			position = lastGroundedPosition;
			velocity = {0.0f, 0.0f};
			invulnerabilityTimer.reset();
		};

		game.pushState(std::make_unique<FadeTransitionState>(respawnLogic));

		registry.remove<PendingRespawnComponent>(entity);
	}
}
