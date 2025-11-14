#pragma once
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {
struct JumpSystem {
	static void update(Registry &registry)
	{
		for (auto view = registry.view<JumpComponent, GroundedComponent, VelocityComponent, InputComponent>();
		     const auto entity : view) {
			auto &[jumpStrength, jumpsLeft, maxJumps] = registry.get<JumpComponent>(entity);
			const auto &[grounded] = registry.get<GroundedComponent>(entity);
			auto &[velocity] = registry.get<VelocityComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);
			if (grounded) {
				jumpsLeft = maxJumps;
			} else if (jumpsLeft == maxJumps) {
				jumpsLeft--;
			}

			if (jumpsLeft && input.jumpPressed) {
				jumpsLeft--;
				velocity.y = -jumpStrength;
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems