#pragma once
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {
struct JumpSystem {
	static void update(Registry &registry)
	{

		for (auto view = registry.view<JumpComponent, GroundedComponent, VelocityComponent, InputComponent>();
		     const auto entity : view) {

			auto &jump = registry.get<JumpComponent>(entity);
			const auto &[grounded] = registry.get<GroundedComponent>(entity);
			auto &[velocity] = registry.get<VelocityComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);
			if (grounded) {
				jump.jumpsLeft = jump.maxJumps;
			}

			if (jump.jumpsLeft && input.jumpPressed && velocity.y <= 0) {
				velocity.y = -jump.jumpStrength;
				jump.jumpsLeft--;
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems