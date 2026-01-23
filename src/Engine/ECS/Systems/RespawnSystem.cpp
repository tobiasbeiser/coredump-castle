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
	for (auto view = registry.view<PendingRespawnComponent>(); const auto entity : view) {
		auto &[delay] = view.get<PendingRespawnComponent>(entity);

		delay.step(std::chrono::duration<float>(deltaTime), false);
		if (delay.isTimeout()) {

			auto respawnLogic = [&registry, entity]() {
				auto lambdaView = registry.view<HealthComponent, PositionComponent, LastGroundedPositionComponent,
				                                VelocityComponent, PendingRespawnComponent>();
				auto &healthComp = lambdaView.get<HealthComponent>(entity);
				auto &[position, previousPosition] = lambdaView.get<PositionComponent>(entity);
				const auto &[lastGroundedPosition] = lambdaView.get<LastGroundedPositionComponent>(entity);
				auto &[velocity] = lambdaView.get<VelocityComponent>(entity);

				registry.remove<PendingRespawnComponent>(entity);

				position = lastGroundedPosition;
				healthComp.invulnerabilityTimer.reset();
				velocity = {0.0f, 0.0f};
			};

			game.pushState(std::make_unique<FadeTransitionState>(respawnLogic));
		}
	}
}