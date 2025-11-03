#pragma once
#include "../Components/Core/PositionComponent.hpp"
#include "../Components/Core/VelocityComponent.hpp"
#include "../Components/Gameplay/InputComponent.hpp"
#include "../Components/Gameplay/MeleeComponent.hpp"
#include "../Components/Gameplay/MovementComponent.hpp"
#include "../Components/Physics/AccelerationComponent.hpp"
#include "../Registry.hpp"
#include <Engine/ECS/Components/Render/RenderComponent.hpp>

namespace GameEngine::ECS::Systems {
class MovementSystem {
  public:
	static void Update(ECS::Registry &registry, float stepSize)
	{
		auto view = registry.view<VelocityComponent, AccelerationComponent, MovementComponent, InputComponent>();
		for (auto entity : view) {
			auto &vel = view.get<VelocityComponent>(entity);
			auto &acc = view.get<AccelerationComponent>(entity);
			auto &move = view.get<MovementComponent>(entity);
			auto &input = view.get<InputComponent>(entity);

			bool isAttacking = false;
			if (registry.has<MeleeComponent>(entity)) {
				auto &melee = registry.get<MeleeComponent>(entity);
				isAttacking = melee.isAttacking;
			}

			if (input.horizontalDirection && !isAttacking) {

				if ((vel.velocity.x > 0 && input.horizontalDirection < 0)
				    || (vel.velocity.x < 0 && input.horizontalDirection > 0)) {
					// Directional change, apply stronger acceleration
					acc.acceleration.x = input.horizontalDirection * move.accelerationRate * 7.5f;
				} else {
					acc.acceleration.x = input.horizontalDirection * move.accelerationRate;
				}
			} else {
				// decelerate
				if (vel.velocity.x) {
					const float factor = vel.velocity.x > 0 ? -2.5f : 2.5f;
					const float amount = factor * move.accelerationRate * stepSize;
					if (std::abs(vel.velocity.x) < std::abs(amount)) {
						acc.acceleration.x = 0;
						vel.velocity.x = 0;
					} else {
						acc.acceleration.x += amount;
						vel.velocity.x += amount;
					}
				}
			}

			if (registry.has<RenderComponent>(entity) && vel.velocity.x) {
				registry.get<RenderComponent>(entity).flipX = vel.velocity.x < 0;
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems