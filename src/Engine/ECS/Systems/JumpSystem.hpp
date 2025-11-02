#pragma once
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {
struct JumpSystem {
	static void Update(Registry &registry)
	{

		for (auto view = registry.view<JumpComponent, GroundedComponent, VelocityComponent, InputComponent>();
		     const auto entity : view) {

			auto &jump = registry.get<JumpComponent>(entity);
			const auto &[grounded] = registry.get<GroundedComponent>(entity);
			auto &[velocity] = registry.get<VelocityComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);
			if (grounded) {
				if (jump.allowDoubleJump) {
					jump.jumpsLeft = jump.maxJumps;
				} else {
					jump.jumpsLeft = 1;
				}
			}

			if (jump.jumpsLeft && input.jumpPressed && velocity.y <= 0) {
				velocity.y = -jump.jumpStrength;
				jump.jumpsLeft--;
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems