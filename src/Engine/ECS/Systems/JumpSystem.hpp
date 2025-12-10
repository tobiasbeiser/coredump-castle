#pragma once
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct JumpSystem {
	static void update(Registry &registry, const float deltaTime)
	{

		for (auto view = registry.view<JumpComponent, GroundedComponent, VelocityComponent, InputComponent>();
		     const auto entity : view) {

			auto &[jumpStrength, jumpsLeft, maxJumps, coyoteTimer] = registry.get<JumpComponent>(entity);
			const auto &[grounded] = registry.get<GroundedComponent>(entity);
			auto &[velocity] = registry.get<VelocityComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);

			coyoteTimer.step(std::chrono::duration<float>(deltaTime), false);
			if (grounded) {
				coyoteTimer.reset();
			}
			if (!coyoteTimer.isTimeout()) {
				jumpsLeft = maxJumps;
			} else if (jumpsLeft == maxJumps) {
				jumpsLeft--; // leave ground without jumping
			}

			if (jumpsLeft && input.jumpPressed) {
				const float multiplier = !coyoteTimer.isTimeout() ? 1.0f : 0.7f;
				velocity.y = -jumpStrength * multiplier;
				jumpsLeft--;
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems