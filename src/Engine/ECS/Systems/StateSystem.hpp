#pragma once
#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"

#include <Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp>
#include <Engine/ECS/Systems/Combat/AttackSystem.hpp>
#include <iostream>

namespace GameEngine::ECS::Systems {
class StateSystem {
  public:
	static void update(Registry &registry)
	{

		for (auto movementView =
		         registry
		             .view<StateComponent, VelocityComponent, GroundedComponent, AttackComponent, HealthComponent>();
		     const auto entity : movementView) {
			auto &state = movementView.get<StateComponent>(entity);

			if (state.state == State::Dead) {
				continue;
			}

			if (state.state == State::Hurting) {
				if (registry.has<AnimationComponent>(entity)) {
					const auto &anim = registry.get<AnimationComponent>(entity);
					if (anim.hasAnimation(AnimationType::Hurting)) {
						const auto &health = registry.get<HealthComponent>(entity);
						if (!health.invulnerabilityTimer.isTimeout()) {
							continue;
						}
					}
				}
			}

			if (const auto &attack = movementView.get<AttackComponent>(entity); AttackSystem::isAttacking(attack)) {
				state.state = State::Attacking;
				continue;
			}

			const auto &vel = movementView.get<VelocityComponent>(entity);
			const auto &ground = movementView.get<GroundedComponent>(entity);

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