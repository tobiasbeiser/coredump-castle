#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/GravityComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include <glm/glm.hpp>

namespace GameEngine::ECS::Systems {
class PhysicsSystem {
  public:
	static void Update(Registry &registry, float timeStep)
	{
		// Gravity

		auto viewGravity = registry.view<VelocityComponent, GravityComponent>().exclude<InactiveTag>();
		for (auto entity : viewGravity) {
			auto &vel = viewGravity.get<VelocityComponent>(entity);
			auto &grav = viewGravity.get<GravityComponent>(entity);
			vel.velocity += glm::vec2(0, grav.gravity) * timeStep;
		}

		auto view = registry.view<PositionComponent, VelocityComponent, AccelerationComponent, MovementComponent>()
		                .exclude<InactiveTag>();
		for (auto entity : view) {
			auto &pos = view.get<PositionComponent>(entity);
			auto &vel = view.get<VelocityComponent>(entity);
			auto &acc = view.get<AccelerationComponent>(entity);
			auto &move = view.get<MovementComponent>(entity);

			vel.velocity += acc.acceleration * timeStep;
			if (std::abs(vel.velocity.x) > move.maxSpeedX) {
				vel.velocity.x = move.maxSpeedX * (vel.velocity.x > 0 ? 1 : -1);
			}
			pos.position += vel.velocity * timeStep;
			pos.previousPosition = pos.position;
		};
	};
};
} // namespace GameEngine::ECS::Systems
