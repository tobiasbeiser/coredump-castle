#pragma once
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {
class StateSystem {
  public:
	static void Update(Registry &registry)
	{
		auto view = registry.view<StateComponent, VelocityComponent, GroundedComponent>();
		for (auto entity : view) {
			auto &state = view.get<StateComponent>(entity);
			auto &vel = view.get<VelocityComponent>(entity);
			auto &ground = view.get<GroundedComponent>(entity);
			if (!ground.grounded) {
				if (vel.velocity.y < 0) {
					state.state = States::Jumping;
				} else {
					state.state = States::Falling;
				}
			} else {
				if (std::abs(vel.velocity.x) > 0.1f) {
					state.state = States::Walking;
				} else if (state.state == States::Attacking) {
					continue;
				} else {
					state.state = States::Idle;
				}
			}

			if (DebugSystem::Instance().player.showState) {
				DebugSystem::Debug("Entity " + std::to_string(entity) + " State: " + std::to_string(state.state));
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems