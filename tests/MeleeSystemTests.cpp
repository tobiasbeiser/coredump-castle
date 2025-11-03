#include "Engine/ECS/Systems/MeleeSystem.hpp"
#include <gtest/gtest.h>

using namespace GameEngine::ECS;
using namespace GameEngine::ECS::Systems;

class MeleeSystemTest : public ::testing::Test {
  protected:
	Registry registry;
	Entity entity;
	MeleeComponent *meleeComp;
	InputComponent *inputComp;

	void SetUp() override
	{
		entity = registry.create();
		registry.register_component<MeleeComponent>(entity, MeleeComponent{});
		registry.register_component<InputComponent>(entity, InputComponent{});

		meleeComp = &registry.get<MeleeComponent>(entity);
		inputComp = &registry.get<InputComponent>(entity);
	}
	void TriggerMeleeInput() { inputComp->meleePressed = true; }
};

TEST_F(MeleeSystemTest, StartsAttackOnMeleeInput)
{
	EXPECT_FALSE(meleeComp->isAttacking);

	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	EXPECT_TRUE(meleeComp->isAttacking);
	EXPECT_FALSE(meleeComp->isComboAttack);
	EXPECT_FLOAT_EQ(meleeComp->attackTimer, 0.0f);
	EXPECT_FALSE(inputComp->meleePressed);
}

TEST_F(MeleeSystemTest, RespectsCooldown)
{
	meleeComp->attackCooldownTimer = 1.0f;

	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	EXPECT_FALSE(meleeComp->isAttacking);
	EXPECT_FLOAT_EQ(meleeComp->attackCooldownTimer, 0.9f);
}

TEST_F(MeleeSystemTest, HandlesComboAttack)
{
	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	EXPECT_TRUE(meleeComp->isAttacking);
	EXPECT_FALSE(meleeComp->isComboAttack);

	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.2f);

	EXPECT_TRUE(meleeComp->comboInputReceived);

	MeleeSystem::Update(registry, 0.4f);

	EXPECT_TRUE(meleeComp->isComboAttack);
	EXPECT_FLOAT_EQ(meleeComp->attackDuration, 0.2f);
}

TEST_F(MeleeSystemTest, EndsAttackAfterDuration)
{
	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	EXPECT_TRUE(meleeComp->isAttacking);

	MeleeSystem::Update(registry, 0.6f);

	EXPECT_FALSE(meleeComp->isAttacking);
	EXPECT_FALSE(meleeComp->isComboAttack);
}

TEST_F(MeleeSystemTest, AppliesLongerCooldownAfterCombo)
{
	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.2f);

	// Complete combo attack
	MeleeSystem::Update(registry, 0.4f); // Finish first attack
	MeleeSystem::Update(registry, 0.2f); // Complete combo attack

	EXPECT_FLOAT_EQ(meleeComp->attackCooldownTimer, 1.0f);
}

TEST_F(MeleeSystemTest, AttackWithoutStateComponentDoesNotCrash)
{
	Registry registry;
	Entity entity = registry.create();
	registry.register_component<MeleeComponent>(entity, MeleeComponent{});
	registry.register_component<InputComponent>(entity, InputComponent{});

	auto &input = registry.get<InputComponent>(entity);
	input.meleePressed = true;
	EXPECT_NO_THROW(MeleeSystem::Update(registry, 0.1f));
}

TEST_F(MeleeSystemTest, MissComboInputWindow)
{
	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);
	MeleeSystem::Update(registry, 0.4f);

	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	EXPECT_FALSE(meleeComp->comboInputReceived);
}

TEST_F(MeleeSystemTest, NoAttackShouldResultInIdleState)
{
	MeleeSystem::Update(registry, 0.1f);

	EXPECT_FALSE(meleeComp->isAttacking);
	EXPECT_FALSE(meleeComp->isComboAttack);
}

TEST_F(MeleeSystemTest, CoolDownDoesNotDecreaseWhileAttacking)
{
	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	float initialCooldown = meleeComp->attackCooldownTimer;

	MeleeSystem::Update(registry, 0.2f);
	EXPECT_FLOAT_EQ(meleeComp->attackCooldownTimer, initialCooldown);
}

TEST_F(MeleeSystemTest, ConsecutiveComboAttempsShouldNotChainInfinitely)
{
	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	MeleeSystem::Update(registry, 0.5f); // Finish first attack
	MeleeSystem::Update(registry, 0.2f); // Complete combo attack

	// Attempt another combo input, cooldown should prevent it
	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.1f);

	EXPECT_FALSE(meleeComp->comboInputReceived);
	EXPECT_FALSE(meleeComp->isAttacking);
}

TEST_F(MeleeSystemTest, ZeroDeltaTimeDoesNotAffectState)
{
	TriggerMeleeInput();
	MeleeSystem::Update(registry, 0.0f);

	EXPECT_TRUE(meleeComp->isAttacking);
	EXPECT_FLOAT_EQ(meleeComp->attackTimer, 0.0f);
}
