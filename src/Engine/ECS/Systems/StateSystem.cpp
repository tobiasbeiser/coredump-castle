#include "Engine/ECS/Systems/StateSystem.hpp"

#include "Combat/AttackSystem.hpp"
#include "Combat/HealthSystem.hpp"
#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
namespace {
const std::unordered_map<State, AnimationType> blockingStateAnimations = {
    {State::Hurting, AnimationType::Hurting}, {State::Interacting, AnimationType::Interacting}};
} // namespace

void GameEngine::ECS::Systems::StateSystem::update(Registry &registry)
{
	for (auto view = registry.view<StateComponent>(); const auto entity : view) {
		auto &state = view.get<StateComponent>(entity);

		if (DebugSystem::instance().getPlayerOptions().showState) {
			DebugSystem::debug("Entity " + std::to_string(entity) + " State: " + toString(state.state));
		}

		if (registry.has<HealthComponent>(entity)) {
			const auto &health = registry.get<HealthComponent>(entity);
			if (HealthSystem::isDead(health)) {
				state.state = State::Dead;
				continue;
			}
		}

		if (auto it = blockingStateAnimations.find(state.state); it != blockingStateAnimations.end()) {
			if (registry.has<AnimationComponent>(entity)) {
				if (const auto &anim = registry.get<AnimationComponent>(entity);
				    anim.hasAnimation(it->second)
				    && (anim.getAnimation() != it->second || !anim.hasFinished(it->second))) {
					continue;
				}
			}
		}

		if (registry.has<AttackComponent>(entity)) {
			if (const auto &attack = registry.get<AttackComponent>(entity); AttackSystem::isAttacking(attack)) {
				state.state = State::Attacking;
				continue;
			}
		}

		if (registry.has<VelocityComponent>(entity) && registry.has<GroundedComponent>(entity)) {
			const auto &vel = registry.get<VelocityComponent>(entity);
			const auto &ground = registry.get<GroundedComponent>(entity);

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
		}
	}
}