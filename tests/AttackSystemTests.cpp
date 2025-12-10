#include "Engine/GameEngine.hpp"
#include <Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp>
#include <Engine/ECS/Components/Gameplay/InputComponent.hpp>
#include <Engine/ECS/Systems/CombatSystem/AttackSystem.hpp>

#include <gtest/gtest.h>

class AttackSystemTest : public ::testing::Test {
  protected:
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity entity;

	void SetUp() override
	{
		entity = registry.create();
		AttackComponent attackComp{.attackTimer = Timer{0.6f}, .attackCoolDownTimer = Timer{0.2f}};

		attackComp.attackCoolDownTimer.step(0.2f, false);

		registry.registerComponent<AttackComponent>(entity, attackComp);
		registry.registerComponent<InputComponent>(entity, InputComponent{});
	}

	AttackComponent &getAttack() { return registry.get<AttackComponent>(entity); }
	InputComponent &getInput() { return registry.get<InputComponent>(entity); }

	void triggerAttackInput() { getInput().attackPressed = true; }
};

TEST_F(AttackSystemTest, StartsAttackOnMeleeInput)
{
	const auto &attack = getAttack();
	const auto &input = getInput();

	EXPECT_FALSE(GameEngine::ECS::Systems::AttackSystem::isAttacking(attack));

	triggerAttackInput();
	GameEngine::ECS::Systems::AttackSystem::update(registry, 0.1f);

	EXPECT_TRUE(GameEngine::ECS::Systems::AttackSystem::isAttacking(attack));
	EXPECT_FLOAT_EQ(attack.attackTimer.getTime(), 0.1f);
	ASSERT_FLOAT_EQ(attack.attackCoolDownTimer.getTime(), 0.2f);
	EXPECT_TRUE(input.attackPressed);
}

TEST_F(AttackSystemTest, RespectsCooldown)
{
	auto &attack = getAttack();

	attack.attackCoolDownTimer.reset();

	triggerAttackInput(); // not possible to attack in a cooldown
	GameEngine::ECS::Systems::AttackSystem::update(registry, 0.1f);

	EXPECT_FALSE(GameEngine::ECS::Systems::AttackSystem::isAttacking(attack));
	EXPECT_FLOAT_EQ(attack.attackCoolDownTimer.getTime(), 0.1f);
}

TEST_F(AttackSystemTest, EndsAttackAfterDuration)
{
	const auto &attack = getAttack();

	triggerAttackInput();
	GameEngine::ECS::Systems::AttackSystem::update(registry, 0.1f);

	EXPECT_TRUE(GameEngine::ECS::Systems::AttackSystem::isAttacking(attack));

	GameEngine::ECS::Systems::AttackSystem::update(registry, 0.6f);

	EXPECT_FALSE(GameEngine::ECS::Systems::AttackSystem::isAttacking(attack));
}

TEST_F(AttackSystemTest, AttackWithoutStateComponentDoesNotCrash)
{
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity entity = registry.create();
	registry.registerComponent<AttackComponent>(entity, AttackComponent{});
	registry.registerComponent<InputComponent>(entity, InputComponent{});

	auto &input = registry.get<InputComponent>(entity);
	input.attackPressed = true;
	EXPECT_NO_THROW(GameEngine::ECS::Systems::AttackSystem::update(registry, 0.1f));
}

TEST_F(AttackSystemTest, NoAttackShouldResultInIdleState)
{
	const auto &attack = getAttack();

	GameEngine::ECS::Systems::AttackSystem::update(registry, 0.1f);

	EXPECT_FALSE(GameEngine::ECS::Systems::AttackSystem::isAttacking(attack));
}

TEST_F(AttackSystemTest, CoolDownDoesNotDecreaseWhileAttacking)
{
	auto &attack = getAttack();

	triggerAttackInput();
	GameEngine::ECS::Systems::AttackSystem::update(registry, 0.1f);

	const float initialCooldown = attack.attackCoolDownTimer.getTime();

	GameEngine::ECS::Systems::AttackSystem::update(registry, 0.2f);
	EXPECT_FLOAT_EQ(attack.attackCoolDownTimer.getTime(), initialCooldown);
}

TEST_F(AttackSystemTest, ZeroDeltaTimeDoesNotAffectState)
{
	const auto &attack = getAttack();

	triggerAttackInput();
	GameEngine::ECS::Systems::AttackSystem::update(registry, 0.0f);

	EXPECT_FALSE(GameEngine::ECS::Systems::AttackSystem::isAttacking(attack));
	EXPECT_FLOAT_EQ(attack.attackTimer.getTime(), 0.0f);
}
