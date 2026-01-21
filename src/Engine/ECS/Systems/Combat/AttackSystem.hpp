#pragma once
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Game/Factories/EffectFactory.hpp"

namespace GameEngine::ECS::Systems {
class AttackSystem {
  public:
	static void update(Registry &registry, const EffectFactory &effectFactory, const float deltaTime)
	{
		for (auto view = registry.view<AttackComponent, InputComponent>(); const auto entity : view) {
			auto &attack = registry.get<AttackComponent>(entity);
			const auto &input = registry.get<InputComponent>(entity);

			updateCoolDown(attack, deltaTime);

			if (input.attackPressed && !isAttacking(attack) && isCoolDown(attack)) {
				startAttack(attack, deltaTime);
				effectFactory.createAttackEffect(entity);
			} else if (isAttacking(attack)) {
				processAttack(attack, deltaTime);

				if (attack.attackTimer.isTimeout()) {
					endAttack(attack);
				}
			}
		}
	}

	static bool isAttacking(const AttackComponent &attack)
	{
		return attack.attackTimer.getTime() > std::chrono::duration<float>::zero() && !attack.attackTimer.isTimeout();
	}

  private:
	static bool isCoolDown(const AttackComponent &attack)
	{
		return attack.attackCoolDownTimer.getTime() >= attack.attackCoolDownTimer.getLength();
	}

	static void updateCoolDown(AttackComponent &attack, const float deltaTime)
	{
		if (!attack.attackCoolDownTimer.isTimeout() && !isAttacking(attack)) {
			attack.attackCoolDownTimer.step(std::chrono::duration<float>(deltaTime), false);
		}
	}

	static void startAttack(AttackComponent &attack, const float deltaTime)
	{
		attack.attackTimer.reset();
		attack.attackTimer.step(std::chrono::duration<float>(deltaTime), false);
	}

	static void processAttack(AttackComponent &attack, const float deltaTime)
	{
		attack.attackTimer.step(std::chrono::duration<float>(deltaTime), false);
	}

	static void endAttack(AttackComponent &attack)
	{
		attack.attackTimer.reset();
		attack.attackCoolDownTimer.reset();
	}
};

} // namespace GameEngine::ECS::Systems
