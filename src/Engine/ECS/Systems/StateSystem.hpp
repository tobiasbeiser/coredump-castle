#pragma once
#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MeleeComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include <cmath>

namespace GameEngine::ECS::Systems {
class StateSystem {
  public:
	static void Update(Registry &registry)
	{
		auto meleeView = registry.view<StateComponent, MeleeComponent>();
		for (auto entity : meleeView) {
			auto &state = meleeView.get<StateComponent>(entity);
			auto &melee = meleeView.get<MeleeComponent>(entity);

			if (melee.isAttacking) {
				if (melee.isComboAttack) {
					state.state = States::ConsecutiveAttacking;
				} else {
					state.state = States::Attacking;
				}
			}
		}

		auto movementView = registry.view<StateComponent, VelocityComponent, GroundedComponent>();
		for (auto entity : movementView) {
			auto &state = movementView.get<StateComponent>(entity);
			auto &vel = movementView.get<VelocityComponent>(entity);
			auto &ground = movementView.get<GroundedComponent>(entity);

			// melee system takes priority
			if (registry.has<MeleeComponent>(entity)) {
				auto &melee = registry.get<MeleeComponent>(entity);
				if (melee.isAttacking) {
					continue;
				}
			}

			if (!ground.grounded) {
				if (vel.velocity.y < 0) {
					state.state = States::Jumping;
				} else {
					state.state = States::Falling;
				}
			} else {
				if (std::abs(vel.velocity.x) > 0.1f) {
					state.state = States::Walking;
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