#pragma once
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MeleeComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct MeleeSystem {
	static void Update(Registry &registry, const float deltaTime)
	{
		for (auto view = registry.view<MeleeComponent, InputComponent>(); const auto entity : view) {
			auto &melee = registry.get<MeleeComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);

			UpdateCoolDown(melee, deltaTime);

			if (input.meleePressed && !melee.isAttacking && melee.attackCooldownTimer <= 0.0f) {
				StartAttack(melee);
			} else if (melee.isAttacking) {
				ProcessAttack(melee, input, deltaTime);

				if (melee.attackTimer >= melee.attackDuration) {
					if (melee.comboInputReceived) {
						StartComboAttack(melee);
					} else {
						EndAttack(melee);
					}
				}
			}
		}
	}

  private:
	static void UpdateCoolDown(MeleeComponent &melee, float deltaTime)
	{
		if (melee.attackCooldownTimer > 0.0f && !melee.isAttacking) {
			melee.attackCooldownTimer -= deltaTime;
		}
	}

	static void StartAttack(MeleeComponent &melee)
	{
		melee.isAttacking = true;
		melee.isComboAttack = false;
		melee.attackTimer = 0.0f;
	}

	static void ProcessAttack(MeleeComponent &melee, const InputComponent &input, const float deltaTime)
	{
		melee.attackTimer += deltaTime;

		if (!melee.isComboAttack && input.meleePressed && melee.attackTimer < melee.comboInputWindow) {
			melee.comboInputReceived = true;
		}
	}

	static void StartComboAttack(MeleeComponent &melee)
	{
		melee.isAttacking = true;
		melee.isComboAttack = true;
		melee.attackTimer = 0.0f;
		melee.attackDuration = melee.comboAttackDuration;
		melee.comboInputReceived = false;
	}

	static void EndAttack(MeleeComponent &melee)
	{
		melee.isAttacking = false;
		melee.attackTimer = 0.0f;
		melee.attackDuration = 0.6f; // Reset to base attack duration
		if (melee.isComboAttack) {
			melee.attackCooldownTimer = 0.5f;
		} else {
			melee.attackCooldownTimer = 0.2f;
		}
		melee.isComboAttack = false;
		melee.comboInputReceived = false;
	}
};

} // namespace GameEngine::ECS::Systems
