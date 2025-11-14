#pragma once
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {
struct JumpSystem {
	static void update(Registry &registry, const float deltaTime)
	{

		for (auto view = registry.view<JumpComponent, GroundedComponent, VelocityComponent, InputComponent>();
		     const auto entity : view) {

			auto &[jumpStrength, jumpsLeft, maxJumps, jumpInitiated, coyoteTimer] = registry.get<JumpComponent>(entity);
			const auto &[grounded] = registry.get<GroundedComponent>(entity);
			auto &[velocity] = registry.get<VelocityComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);

			if (grounded) {
				jumpInitiated = input.jumpPressed;
				jumpsLeft = maxJumps;
				coyoteTimer.reset();
			} else{
				if (!jumpInitiated && jumpsLeft > 0 && !coyoteTimer.isTimeout()) {
					coyoteTimer.step(deltaTime, false);
					jumpsLeft = 1;
				} else if (coyoteTimer.isTimeout()) {
					jumpsLeft = 0;
				}
			}

			if (jumpsLeft && input.jumpPressed) {
				velocity.y = -jumpStrength;
				jumpsLeft--;
			}
		}

	}
};

} // namespace GameEngine::ECS::Systems