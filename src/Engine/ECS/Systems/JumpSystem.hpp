#pragma once
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {
struct JumpSystem {
	static void update(Registry &registry)
	{
		auto view = registry.view<JumpComponent, GroundedComponent, VelocityComponent, InputComponent>();
		for (auto entity : view) {
			auto &jump = registry.get<JumpComponent>(entity);
			auto &grounded = registry.get<GroundedComponent>(entity);
			auto &vel = registry.get<VelocityComponent>(entity);
			auto &input = registry.get<InputComponent>(entity);
			if (grounded.grounded) {
				jump.jumpsLeft = jump.maxJumps;
			}

			if (jump.jumpsLeft && input.jumpPressed) {
				jump.jumpsLeft--;
				vel.velocity.y = -jump.jumpStrength;
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems