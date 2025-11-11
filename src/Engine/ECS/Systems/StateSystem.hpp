#pragma once
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MeleeComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {
class StateSystem {
  public:
	static void update(Registry &registry)
	{
		auto meleeView = registry.view<StateComponent, MeleeComponent>();
		for (auto entity : meleeView) {
			auto &state = meleeView.get<StateComponent>(entity);
			auto &melee = meleeView.get<MeleeComponent>(entity);

			if (melee.isAttacking) {
				if (melee.isComboAttack) {
					state.state = State::ConsecutiveAttacking;
				} else {
					state.state = State::Attacking;
				}
			}
		}

		auto view = registry.view<StateComponent, VelocityComponent, GroundedComponent>();
		for (auto entity : view) {
			auto &state = view.get<StateComponent>(entity);
			auto &vel = view.get<VelocityComponent>(entity);
			auto &ground = view.get<GroundedComponent>(entity);

			// melee system takes priority
			if (registry.has<MeleeComponent>(entity)) {
				auto &melee = registry.get<MeleeComponent>(entity);
				if (melee.isAttacking) {
					continue;
				}
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