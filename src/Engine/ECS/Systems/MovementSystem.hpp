#pragma once
#include "../Components/Core/VelocityComponent.hpp"
#include "../Components/Gameplay/InputComponent.hpp"
#include "../Components/Gameplay/MovementComponent.hpp"
#include "../Components/Physics/AccelerationComponent.hpp"
#include "../Registry.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"

namespace GameEngine::ECS::Systems {
class MovementSystem {
  public:
	static void update(Registry &registry, const float stepSize)
	{
		for (auto &view = registry.view<VelocityComponent, AccelerationComponent, MovementComponent, InputComponent>()
		                      .exclude<InactiveTag>();
		     const auto entity : view) {
			auto &[velocity] = view.get<VelocityComponent>(entity);
			auto &[acceleration] = view.get<AccelerationComponent>(entity);
			const auto &[accelerationRate, maxSpeedX] = view.get<MovementComponent>(entity);

			if (const auto &input = view.get<InputComponent>(entity); input.horizontalDirection != 0.0f) {
				if ((velocity.x > 0 && input.horizontalDirection < 0)
				    || (velocity.x < 0 && input.horizontalDirection > 0)) {
					// Directional change, apply stronger acceleration
					acceleration.x = input.horizontalDirection * accelerationRate * 7.5f;
				} else {
					acceleration.x = input.horizontalDirection * accelerationRate;
				}
			} else {
				// decelerate
				if (velocity.x) {
					const float factor = velocity.x > 0 ? -2.5f : 2.5f;
					const float amount = factor * accelerationRate * stepSize;
					if (std::abs(velocity.x) < std::abs(amount)) {
						acceleration.x = 0;
						velocity.x = 0;
					} else {
						acceleration.x += amount;
						velocity.x += amount;
					}
				}
			}

			if (registry.has<RenderComponent>(entity) && velocity.x) {
				registry.get<RenderComponent>(entity).flipX = velocity.x < 0;
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems