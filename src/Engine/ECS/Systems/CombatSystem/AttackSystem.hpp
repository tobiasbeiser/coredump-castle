#pragma once
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems {
struct AttackSystem {
	static void update(Registry &registry, const float deltaTime)
	{
		// Handle player attacks (with InputComponent and PlayerTag)
		for (auto view = registry.view<PlayerTag, AttackComponent, InputComponent>(); const auto entity : view) {
			auto &attack = registry.get<AttackComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);

			updateCoolDown(attack, deltaTime);

			if (input.attackPressed && !isAttacking(attack) && isCoolDown(attack)) {
				startAttack(attack, deltaTime);
			} else if (isAttacking(attack)) {
				processAttack(attack, deltaTime);

				if (attack.attackTimer.isTimeout()) {
					endAttack(attack);
				}
			}
		}

		// Handle enemy attacks (with EnemyTag, without InputComponent)
		for (auto view = registry.view<EnemyTag, AttackComponent>(); const auto entity : view) {
			auto &attack = registry.get<AttackComponent>(entity);

			updateCoolDown(attack, deltaTime);

			// AI/Enemy attack logic will be triggered by other systems
			// (e.g., CombatCollisionSystem can call triggerAttack)
			if (isAttacking(attack)) {
				processAttack(attack, deltaTime);

				if (attack.attackTimer.isTimeout()) {
					endAttack(attack);
				}
			}
		}
	}

	static bool isAttacking(const AttackComponent &attack)
	{
		return attack.attackTimer.getTime() > 0.0f && !attack.attackTimer.isTimeout();
	}

	// Allow external systems to trigger attacks (for AI/enemies)
	static void triggerAttack(AttackComponent &attack, const float deltaTime)
	{
		if (!isAttacking(attack) && isCoolDown(attack)) {
			startAttack(attack, deltaTime);
		}
	}

  private:
	static bool isCoolDown(const AttackComponent &attack)
	{
		return attack.attackCoolDownTimer.getTime() >= attack.attackCoolDownTimer.getLength();
	}

	static void updateCoolDown(AttackComponent &attack, const float deltaTime)
	{
		if (!attack.attackCoolDownTimer.isTimeout() && !isAttacking(attack)) {
			attack.attackCoolDownTimer.step(deltaTime, false);
		}
	}

	static void startAttack(AttackComponent &attack, const float deltaTime)
	{
		attack.attackTimer.reset();
		attack.attackTimer.step(deltaTime, false);
	}

	static void processAttack(AttackComponent &attack, const float deltaTime)
	{
		attack.attackTimer.step(deltaTime, false);
	}

	static void endAttack(AttackComponent &attack)
	{
		attack.attackTimer.reset();
		attack.attackCoolDownTimer.reset();
	}
};

} // namespace GameEngine::ECS::Systems
