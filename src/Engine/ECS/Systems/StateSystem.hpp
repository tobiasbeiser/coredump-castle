#pragma once
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ComboComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems {
class StateSystem {
  public:
	static void update(Registry &registry)
	{
		// auto meleeView = registry.view<StateComponent, AttackComponent>();
		// for (auto entity : meleeView) {
		// 	auto &state = meleeView.get<StateComponent>(entity);
		// 	auto &melee = meleeView.get<AttackComponent>(entity);
		//
		// 	if (melee.isAttacking) {
		// 		if (melee.isComboAttack) {
		// 			state.state = States::ConsecutiveAttacking;
		// 		} else {
		// 			state.state = States::Attacking;
		// 		}
		// 	}
		// }


		auto movementView = registry.view<StateComponent, VelocityComponent, GroundedComponent>();
		for (auto entity : movementView) {
			auto &state = movementView.get<StateComponent>(entity);
			auto &vel = movementView.get<VelocityComponent>(entity);
			auto &ground = movementView.get<GroundedComponent>(entity);
			auto &attackState = movementView.get<AttackStateComponent>(entity);
			auto &combo = movementView.get<ComboComponent>(entity);

			if (attackState.isAttacking) {
				if (combo.isComboAttack) {
					state.state = State::ConsecutiveAttacking;
				} else {
					state.state = State::Attacking;
				}
			}
			if (registry.has<AttackStateComponent>(entity)) {
				auto &melee = registry.get<AttackStateComponent>(entity);
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