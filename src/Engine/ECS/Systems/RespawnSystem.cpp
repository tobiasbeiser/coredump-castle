#include "RespawnSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/PendingRespawnComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/View.hpp"
#include "Game/States/FadeTransitionState.hpp"

void GameEngine::ECS::Systems::RespawnSystem::update(Registry &registry, GameEngine &game, const float deltaTime)
{
	for (auto view = registry.view<PendingRespawnComponent, HealthComponent, PositionComponent,
	                               LastGroundedPositionComponent, VelocityComponent, AccelerationComponent>();
	     const auto entity : view) {
		auto &[delay] = registry.get<PendingRespawnComponent>(entity);

		delay.step(std::chrono::duration<float>(deltaTime), false);
		if (delay.isTimeout()) {
			auto &healthComp = registry.get<HealthComponent>(entity);
			auto &invulnerabilityTimer = healthComp.invulnerabilityTimer;
			auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
			const auto &[lastGroundedPosition] = registry.get<LastGroundedPositionComponent>(entity);
			auto [velocity] = registry.get<VelocityComponent>(entity);

			auto respawnLogic = [&position, &lastGroundedPosition, &invulnerabilityTimer, &velocity]() {
				position = lastGroundedPosition;
				invulnerabilityTimer.reset();
				velocity = {0.0f, 0.0f};
			};

			game.pushState(std::make_unique<FadeTransitionState>(respawnLogic));

			registry.remove<PendingRespawnComponent>(entity);
		}
	}
}