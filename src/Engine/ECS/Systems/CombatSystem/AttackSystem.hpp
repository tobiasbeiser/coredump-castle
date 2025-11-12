#pragma once
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct AttackSystem {
	static void update(Registry &registry, const float deltaTime)
	{
		for (auto view = registry.view<AttackComponent, InputComponent>(); const auto entity : view) {
			auto &attack = registry.get<AttackComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);

			updateCoolDown(attack, deltaTime);

			if (input.attackPressed && !attack.isAttacking && attack.attackCoolDownTimer.getTime() >= attack.attackCoolDownTimer.getLength()) {
				startAttack(attack);
			} else if (attack.isAttacking) {
				processAttack(attack, deltaTime);

				if (attack.attackTimer.isTimeout()) {
					endAttack(attack);
				}
			}
		}
	}

  private:
	static void updateCoolDown(AttackComponent &state, const float deltaTime)
	{
		if (!state.attackCoolDownTimer.isTimeout() && !state.isAttacking) {
			state.attackCoolDownTimer.step(deltaTime, false);
		}
	}

	static void startAttack(AttackComponent &state) { state.isAttacking = true; }

	static void processAttack(AttackComponent &attack, const float deltaTime)
	{
		attack.attackTimer.step(deltaTime, false);
	}

	static void endAttack(AttackComponent &attack)
	{
		attack.isAttacking = false;
		attack.attackTimer.reset();
		attack.attackCoolDownTimer.reset();
	}
};

} // namespace GameEngine::ECS::Systems
