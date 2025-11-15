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

			auto &[jumpStrength, jumpsLeft, maxJumps, jumpInitiated, coyoteTimer] = registry.get<JumpComponent>(entity);
			auto &[grounded] = registry.get<GroundedComponent>(entity);
			auto &[velocity] = registry.get<VelocityComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);

			if (grounded) {
				jumpInitiated = input.jumpPressed;
				jumpsLeft = maxJumps;
				coyoteTimer.reset();
			} else{
				if (!jumpInitiated && jumpsLeft > 0) { //check for jumpsLeft > 0 to see if jump was already consumed
					coyoteTimer.step(deltaTime, false);
					jumpsLeft = coyoteTimer.isTimeout() ? 0 : 1;
				}
			}

			if (jumpsLeft && input.jumpPressed) {
				velocity.y = -jumpStrength;
				jumpsLeft--;
				grounded = false;
			}
		}

	}
};

} // namespace GameEngine::ECS::Systems