#pragma once
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MeleeComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include <Engine/ECS/Components/Core/PositionComponent.hpp>
#include <Engine/ECS/Components/Core/StateComponent.hpp>

namespace GameEngine::ECS::Systems {
struct MeleeSystem {
	static void Update(Registry &registry, float deltaTime)
	{
		auto view = registry.view<MeleeComponent, InputComponent>();
		for (auto entity : view) {
			auto &melee = registry.get<MeleeComponent>(entity);
			auto &input = registry.get<InputComponent>(entity);
			auto &pos = registry.get<PositionComponent>(entity);
			StateComponent *state = nullptr;

			if (registry.has<StateComponent>(entity)) {
				state = &registry.get<StateComponent>(entity);
			}

			UpdateCoolDown(melee, state, deltaTime);

			if (input.meleePressed && !melee.isAttacking && melee.attackCooldown <= 0.0f) {
				StartAttack(melee, input, state);
			} else if (melee.isAttacking) {
				ProcessAttack(melee, input, state, deltaTime);

				if (melee.attackTimer >= melee.attackDuration) {
					if (melee.comboInputReceived) {
						StartComboAttack(melee, input, state);
					} else {
						EndAttack(melee, state);
					}
				}
			}

			// if (registry.has<StateComponent>(entity)) {
			// 	state = &registry.get<StateComponent>(entity);
			// }

			// if (melee.attackCooldown > 0.0f && state->state != States::Attacking
			//     && state->state != States::ConsecutiveAttack) {
			// 	melee.attackCooldown -= deltaTime;
			// }

			// if (input.meleePressed && !melee.isAttacking && melee.attackCooldown <= 0.0f) {
			// 	melee.isAttacking = true;
			// 	melee.attackTimer = 0.0f;
			// 	input.meleePressed = false;
			// 	if (state) {
			// 		registry.get<StateComponent>(entity).state = States::Attacking;
			// 	}

			// } else if (melee.isAttacking) {
			// 	melee.attackTimer += deltaTime;
			// 	if (state && state->state != States::ConsecutiveAttack) {
			// 		registry.get<StateComponent>(entity).state = States::Attacking;
			// 	}

			// 	if (state && state->state == States::Attacking && input.meleePressed
			// 	    && melee.attackTimer < melee.comboInputWindow) {
			// 		melee.comboInputReceived = true;
			// 		input.meleePressed = false;
			// 	}
			// 	if (melee.attackTimer >= melee.attackDuration) {
			// 		if (melee.comboInputReceived) {
			// 			melee.isAttacking = true;
			// 			melee.attackTimer = 0.0f;
			// 			melee.attackDuration = 0.2f;
			// 			melee.comboInputReceived = false;
			// 			if (state) {
			// 				registry.get<StateComponent>(entity).state = States::ConsecutiveAttack;
			// 			}
			// 		} else {
			// 			melee.isAttacking = false;
			// 			melee.attackTimer = 0.0f;
			// 			melee.attackDuration = 0.5f;
			// 			if (state->state == States::ConsecutiveAttack) {
			// 				melee.attackCooldown = 1.0f;
			// 			} else {
			// 				melee.attackCooldown = 0.5f;
			// 			}

			// 			if (registry.has<StateComponent>(entity)) {
			// 				registry.get<StateComponent>(entity).state = States::Idle;
			// 			}
			// 		}
			// 	}
			// }
		}
	}

  private:
	static void UpdateCoolDown(MeleeComponent &melee, StateComponent *state, float deltaTime)
	{
		if (melee.attackCooldown > 0.0f
		    && (!state || (state->state != States::Attacking && state->state != States::ConsecutiveAttack))) {
			melee.attackCooldown -= deltaTime;
		}
	}

	static void StartAttack(MeleeComponent &melee, InputComponent &input, StateComponent *state)
	{
		melee.isAttacking = true;
		melee.attackTimer = 0.0f;
		input.meleePressed = false;
		if (state) {
			state->state = States::Attacking;
		}
	}

	static void ProcessAttack(MeleeComponent &melee, InputComponent &input, StateComponent *state, float deltaTime)
	{
		melee.attackTimer += deltaTime;
		if (state && state->state != States::ConsecutiveAttack) {
			state->state = States::Attacking;
		}

		if (state && state->state == States::Attacking && input.meleePressed
		    && melee.attackTimer < melee.comboInputWindow) {
			melee.comboInputReceived = true;
			input.meleePressed = false;
		}
	}

	static void StartComboAttack(MeleeComponent &melee, InputComponent &input, StateComponent *state)
	{
		melee.isAttacking = true;
		melee.attackTimer = 0.0f;
		melee.attackDuration = 0.2f;
		melee.comboInputReceived = false;
		if (state) {
			state->state = States::ConsecutiveAttack;
		}
	}

	static void EndAttack(MeleeComponent &melee, StateComponent *state)
	{
		melee.isAttacking = false;
		melee.attackTimer = 0.0f;
		melee.attackDuration = 0.5f;
		if (state && state->state == States::ConsecutiveAttack) {
			melee.attackCooldown = 1.0f;
		} else {
			melee.attackCooldown = 0.5f;
		}

		if (state) {
			state->state = States::Idle;
		}
	}
};

} // namespace GameEngine::ECS::Systems
