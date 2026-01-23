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

		if (const auto health = registry.tryGet<HealthComponent>(entity)) {
			if (HealthSystem::isDead(health->get())) {
				state.state = State::Dead;
				continue;
			}
		}

		if (auto it = blockingStateAnimations.find(state.state); it != blockingStateAnimations.end()) {
			if (const auto anim = registry.tryGet<AnimationComponent>(entity)) {
				if (anim->get().hasAnimation(it->second)
				    && (anim->get().getAnimation() != it->second || !anim->get().hasFinished(it->second))) {
					continue;
				}
			}
		}

		if (registry.has<AttackComponent>(entity)) {
			if (const auto &attack = registry.tryGet<AttackComponent>(entity);
			    attack && AttackSystem::isAttacking(attack->get())) {
				state.state = State::Attacking;
				continue;
			}
		}

		const auto vel = registry.tryGet<VelocityComponent>(entity);
		const auto ground = registry.tryGet<GroundedComponent>(entity);

		if (vel && ground) {
			if (!ground->get().grounded) {
				if (vel->get().velocity.y < 0) {
					state.state = State::Jumping;
				} else {
					state.state = State::Falling;
				}
			} else {
				if (std::abs(vel->get().velocity.x) > 0.1f) {
					state.state = State::Walking;
				} else {
					state.state = State::Idle;
				}
			}
		}
	}
}