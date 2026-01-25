#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Game/Factories/EffectFactory.hpp"

#include <functional>

namespace GameEngine::ECS::Systems::AttackSystem {

using AttackCallback = std::function<void(Entity)>;

bool isAttacking(const AttackComponent &attack)
{
	return attack.attackTimer.getTime() > std::chrono::duration<float>::zero() && !attack.attackTimer.isTimeout();
}

namespace {

bool isCoolDown(const AttackComponent &attack)
{
	return attack.attackCoolDownTimer.getTime() >= attack.attackCoolDownTimer.getLength();
}

void updateCoolDown(AttackComponent &attack, const float deltaTime)
{
	if (!attack.attackCoolDownTimer.isTimeout() && !isAttacking(attack)) {
		attack.attackCoolDownTimer.step(std::chrono::duration<float>(deltaTime), false);
	}
}

void startAttack(AttackComponent &attack, const float deltaTime)
{
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(deltaTime), false);
}

void processAttack(AttackComponent &attack, const float deltaTime)
{
	attack.attackTimer.step(std::chrono::duration<float>(deltaTime), false);
}

void endAttack(AttackComponent &attack)
{
	attack.attackTimer.reset();
	attack.attackCoolDownTimer.reset();
}

void updateDamageState(Registry &registry, Entity entity, AttackComponent &attack)
{
	bool usingFrameData = false;
	if (const auto anim = registry.tryGet<AnimationComponent>(entity)) {
		const auto currentAnim = anim->get().getAnimation();
		if (anim->get().usesDamageFrames(currentAnim)) {
			attack.isDamageActive = anim->get().isDamageActive(currentAnim);
			usingFrameData = true;
		}
	}

	if (!usingFrameData) {
		if (attack.activeWindUpLimit > 0.0f) {
			const float progress = attack.attackTimer.getTime().count() / attack.attackTimer.getLength().count();
			attack.isDamageActive = progress > attack.activeWindUpLimit;
		} else {
			attack.isDamageActive = true;
		}
	}
}

}; // namespace

void update(Registry &registry, const float deltaTime, const AttackCallback &onAttack)
{
	PROFILE_SCOPE("AttackSystem");
	for (auto view = registry.view<AttackComponent, InputComponent>(); const auto entity : view) {
		auto &attack = view.get<AttackComponent>(entity);
		const auto &input = view.get<InputComponent>(entity);

		updateCoolDown(attack, deltaTime);

		if (input.attackPressed && !isAttacking(attack) && isCoolDown(attack)) {
			startAttack(attack, deltaTime);
			onAttack(entity);
		} else if (isAttacking(attack)) {
			processAttack(attack, deltaTime);
		}

		if (isAttacking(attack)) {
			updateDamageState(registry, entity, attack);

			if (attack.attackTimer.isTimeout()) {
				endAttack(attack);
			}
		}
	}
}
} // namespace GameEngine::ECS::Systems::AttackSystem
