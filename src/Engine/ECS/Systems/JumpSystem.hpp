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

			auto &jump = registry.get<JumpComponent>(entity);
			const auto &[grounded] = registry.get<GroundedComponent>(entity);
			auto &[velocity] = registry.get<VelocityComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);

			if (grounded) {
				jump.jumpInitiated = false;
				if (input.jumpPressed) {
					jump.jumpInitiated = true;
				}
				jump.jumpsLeft = jump.maxJumps;
				jump.coyoteTimer.reset();
			} else{
				if (!jump.jumpInitiated && jump.jumpsLeft > 0 && !jump.coyoteTimer.isTimeout()) {
					jump.coyoteTimer.step(deltaTime, false);
					jump.jumpsLeft = 1;
				} else if (jump.coyoteTimer.isTimeout()) {
					jump.jumpsLeft = 0;
				}
			}

			if (jump.jumpsLeft && input.jumpPressed) {
				velocity.y = -jump.jumpStrength;
				jump.jumpsLeft--;
			}
		}

	}
};

} // namespace GameEngine::ECS::Systems