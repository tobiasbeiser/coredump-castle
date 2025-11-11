#pragma once
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ComboComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct PlayerAttackSystem {
	static void Update(Registry &registry, const float deltaTime)
	{
		for (auto view = registry.view<AttackComponent, AttackStateComponent, ComboComponent, InputComponent>(); const auto entity : view) {
			auto &attack = registry.get<AttackComponent>(entity);
			auto &state = registry.get<AttackStateComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);
			auto &combo = registry.get<ComboComponent>(entity);

			UpdateCoolDown(state, deltaTime);

			if (input.meleePressed && !state.isAttacking && state.attackCooldownTimer <= 0.0f) {
				StartAttack(state, combo);
			} else if (state.isAttacking) {
				ProcessAttack(state, attack, combo, input, deltaTime);

				const float currentAttackDuration = combo.isComboAttack ? combo.comboAttackDuration : attack.attackDuration;
				if (state.attackTimer >= currentAttackDuration) {
					if (combo.comboInputReceived) {
						StartComboAttack(state, combo);
					} else {
						EndAttack(state, combo);
					}
				}
			}
		}
	}

  private:
	static void UpdateCoolDown(AttackStateComponent &state,const float deltaTime)
	{
		if (state.attackCooldownTimer > 0.0f && !state.isAttacking) {
			state.attackCooldownTimer -= deltaTime;
		}
	}

	static void StartAttack(AttackStateComponent &state, ComboComponent &combo)
	{
		state.isAttacking = true;
		combo.isComboAttack = false;
		state.attackTimer = 0.0f;
		combo.comboInputReceived = false;
	}

	static void ProcessAttack(AttackStateComponent &state, const AttackComponent &attack, ComboComponent &combo, const InputComponent &input, const float deltaTime)
	{
		state.attackTimer += deltaTime;

		if (!combo.isComboAttack && input.meleePressed && state.attackTimer < combo.comboInputWindow) {
			combo.comboInputReceived = true;
		}
	}

	static void StartComboAttack(AttackStateComponent &state, ComboComponent &combo)
	{
		state.isAttacking = true;
		combo.isComboAttack = true;
		state.attackTimer = 0.0f;
		combo.comboInputReceived = false;
	}

	static void EndAttack(AttackStateComponent &state, ComboComponent &combo)
	{
		state.isAttacking = false;
		state.attackTimer = 0.0f;
		if (combo.isComboAttack) {
			state.attackCooldownTimer = 0.5f;
		} else {
			state.attackCooldownTimer = 0.2f;
		}
		combo.isComboAttack = false;
		combo.comboInputReceived = false;
	}
};

} // namespace GameEngine::ECS::Systems
