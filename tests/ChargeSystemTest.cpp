#include <gtest/gtest.h>

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp" // Added
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp" // Added
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp" // Added
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Enemy/ChargeSystem.hpp"
#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"

using namespace GameEngine::ECS;

class ChargeSystemTest : public ::testing::Test {
  protected:
	Registry registry;
	Entity player;
	Entity enemy;

	void SetUp() override
	{
		// Create Player
		player = registry.create();
		registry.registerComponent(player, PlayerTag{});
		registry.registerComponent(player, PositionComponent{.position = {100.0f, 100.0f}});
		registry.registerComponent(player, VelocityComponent{});
		registry.registerComponent(player, TransformComponent{});
		registry.registerComponent(player, StateComponent{});
		registry.registerComponent(player, HurtBoxComponent{.hurtBox = {0, 0, 32, 32}});
		registry.registerComponent(player, GroundedComponent{.grounded = true});

		// Create Enemy
		enemy = registry.create();
		registry.registerComponent(enemy, EnemyTag{});                                      // Fixed namespace
		registry.registerComponent(enemy, PositionComponent{.position = {200.0f, 100.0f}}); // 100 units away
		registry.registerComponent(enemy, VelocityComponent{});
		registry.registerComponent(enemy, TransformComponent{});
		registry.registerComponent(enemy, InputComponent{});
		registry.registerComponent(enemy, MovementComponent{});
		registry.registerComponent(enemy, RenderComponent{});
		registry.registerComponent(enemy, HitBoxComponent{.bodyHitBox = {0, 0, 32, 32}});
		registry.registerComponent(enemy, HurtBoxComponent{.hurtBox = {0, 0, 32, 32}});

		// Chase Params
		ChaseComponent chase;
		chase.chaseSpeed = 50.0f;
		chase.detectionWidthRange = 500.0f;
		chase.detectionHeightRange = 100.0f;
		chase.reactionDelay = Timer{std::chrono::duration<float>(0.1f)};
		chase.reactionDelay.step(std::chrono::duration<float>(1.0f), false);
		registry.registerComponent(enemy, chase);

		// Charge Params
		ChargeComponent charge;
		charge.chargeChance = 2.0f;
		charge.chargeCooldown = Timer{std::chrono::duration<float>(0.1f)};
		charge.chargeCooldown.step(std::chrono::duration<float>(1.0f), false);
		charge.chargeTimer = Timer{std::chrono::duration<float>(0.5f)};
		charge.chargeTimer.step(std::chrono::duration<float>(1.0f), true); // Timeout
		registry.registerComponent(enemy, charge);

		// Add AttackComponent (needed for ChaseSystem checks)
		registry.registerComponent(enemy, AttackComponent{});

		// Add PatrolComponent (needed for ChaseSystem view)
		registry.registerComponent(enemy, PatrolComponent{});
	}
};

TEST_F(ChargeSystemTest, TransitionsToChargeState)
{
	// Run ChaseSystem. Should detect player and add ChargeState
	Systems::ChaseSystem::update(registry, 0.1f);

	EXPECT_TRUE(registry.has<ChargeState>(enemy));
	ASSERT_TRUE(registry.tryGet<ChargeComponent>(enemy).has_value());
	EXPECT_TRUE(registry.tryGet<ChargeComponent>(enemy)->get().chargeTimer.isRunning());
}

TEST_F(ChargeSystemTest, UpdatesPositionTowardsPlayer)
{
	// Manually start charge
	registry.registerComponent(enemy, ChargeState{});
	ASSERT_TRUE(registry.tryGet<ChargeComponent>(enemy).has_value());
	auto &c = registry.tryGet<ChargeComponent>(enemy)->get();
	c.chargeTimer.reset();
	c.chargeTimer.step(std::chrono::duration<float>(0.0f), false);

	Systems::ChargeSystem::update(registry, 0.1f);

	// Enemy is at 200, Player at 100. Should move left (negative horizontal direction)

	const auto &input = registry.tryGet<InputComponent>(enemy)->get();
	EXPECT_FLOAT_EQ(input.horizontalDirection, -1.0f);

	const auto &movement = registry.tryGet<MovementComponent>(enemy)->get();
	EXPECT_GT(movement.maxSpeedX, 50.0f); // Should be higher than base chase speed
}

TEST_F(ChargeSystemTest, EndsChargeOnTimeout)
{
	registry.registerComponent(enemy, ChargeState{});
	ASSERT_TRUE(registry.tryGet<ChargeComponent>(enemy).has_value());
	auto &c = registry.tryGet<ChargeComponent>(enemy)->get();
	c.chargeTimer.reset();

	// Advance time past timeout
	// Timer default is 0.8s
	Systems::ChargeSystem::update(registry, 1.0f);

	EXPECT_FALSE(registry.has<ChargeState>(enemy));
}

TEST_F(ChargeSystemTest, HandlesCollisionWithPlayer)
{
	registry.registerComponent(enemy, ChargeState{});
	ASSERT_TRUE(registry.tryGet<ChargeComponent>(enemy).has_value());
	auto &c = registry.tryGet<ChargeComponent>(enemy)->get();
	c.chargeTimer.reset();
	c.chargeTimer.step(std::chrono::duration<float>(0.0f), false);

	// Move enemy inside player
	registry.tryGet<PositionComponent>(enemy)->get().position = {100.0f, 100.0f};

	Systems::ChargeSystem::update(registry, 0.1f);

	// Should trigger collision logic (snap out and pause)

	float newX = registry.tryGet<PositionComponent>(enemy)->get().position.x;
	EXPECT_NE(newX, 100.0f);

	// Should still be in ChargeState (but paused)
	EXPECT_TRUE(registry.has<ChargeState>(enemy));

	// Run again to process pause timeout (0.1s default pause)
	Systems::ChargeSystem::update(registry, 0.2f);

	// Should exit ChargeState
	EXPECT_FALSE(registry.has<ChargeState>(enemy));
}
