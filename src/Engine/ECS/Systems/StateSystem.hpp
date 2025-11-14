#pragma once
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include <Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp>
#include <Engine/ECS/Components/Render/AnimationComponent.hpp>

namespace GameEngine::ECS::Systems {
class StateSystem {
  public:
	static void update(Registry &registry)
	{

		auto movementView = registry.view<StateComponent, VelocityComponent, GroundedComponent, AttackComponent>();
		for (const auto entity : movementView) {
			auto &state = movementView.get<StateComponent>(entity);
			const auto &vel = movementView.get<VelocityComponent>(entity);
			const auto &ground = movementView.get<GroundedComponent>(entity);
			const auto &attack = movementView.get<AttackComponent>(entity);

			if (AttackSystem::isAttacking(attack)) {
				state.state = State::Attacking;
				continue;
			}

			if (!ground.grounded) {
				if (vel.velocity.y < 0) {
					state.state = State::Jumping;
				} else {
					state.state = State::Falling;
				}
			} else {
				if (std::abs(vel.velocity.x) > 0.1f) {
					state.state = State::Walking;
				} else {
					state.state = State::Idle;
				}
			}

			if (DebugSystem::instance().player.showState) {
				DebugSystem::debug("Entity " + std::to_string(entity) + " State: " + toString(state.state));
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems