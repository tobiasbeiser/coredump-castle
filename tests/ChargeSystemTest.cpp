#include <gtest/gtest.h>

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Enemy/ChargeSystem.hpp"

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
		registry.registerComponent(enemy, EnemyTag{});
		registry.registerComponent(enemy, PositionComponent{.position = {200.0f, 100.0f}});
		registry.registerComponent(enemy, VelocityComponent{});
		registry.registerComponent(enemy, TransformComponent{});
		registry.registerComponent(enemy, InputComponent{});
		registry.registerComponent(enemy, MovementComponent{});
		registry.registerComponent(enemy, RenderComponent{});
		registry.registerComponent(enemy, HitBoxComponent{.bodyHitBox = {0, 0, 32, 32}});
		registry.registerComponent(enemy, HurtBoxComponent{.hurtBox = {0, 0, 32, 32}});

		// Chase component
		ChaseComponent chase;
		chase.chaseSpeed = 50.0f;
		chase.detectionRange = 500.0f;
		chase.giveUpRange = 600.0f;
		chase.reactionDelay = Timer{std::chrono::duration<float>(0.1f)};
		chase.reactionDelay.step(std::chrono::duration<float>(1.0f), false);
		registry.registerComponent(enemy, chase);

		// Charge component
		ChargeComponent charge;
		charge.chargeSpeedMultiplier = 2.0f;
		charge.chargeCooldown = Timer{std::chrono::duration<float>(1.0f)};
		charge.chargeCooldown.timeoutNow(); // Ready to charge
		charge.chargeTimer = Timer{std::chrono::duration<float>(0.5f)};
		registry.registerComponent(enemy, charge);

		registry.registerComponent(enemy, AttackComponent{});
	}
};

TEST_F(ChargeSystemTest, ChargesWhenTimerActive)
{
	auto &charge = registry.get<ChargeComponent>(enemy);
	auto &input = registry.get<InputComponent>(enemy);
	
	// Start charge
	charge.chargeDirection = -1.0f; // Towards player
	charge.chargeTimer.reset();
	charge.chargeTimer.step(std::chrono::duration<float>(0.0f), false); // Start timer
	
	Systems::ChargeSystem::updateEntity(registry, enemy, 0.1f);
	
	EXPECT_FLOAT_EQ(input.horizontalDirection, -1.0f);
	EXPECT_TRUE(charge.chargeTimer.isRunning());
}

TEST_F(ChargeSystemTest, EndsChargeOnTimeout)
{
	auto &charge = registry.get<ChargeComponent>(enemy);
	
	// Start charge
	charge.chargeDirection = -1.0f;
	charge.chargeTimer.reset();
	charge.chargeTimer.step(std::chrono::duration<float>(0.0f), false);
	
	// Advance past timeout
	Systems::ChargeSystem::updateEntity(registry, enemy, 1.0f);
	
	EXPECT_FALSE(charge.chargeTimer.isRunning());
}

TEST_F(ChargeSystemTest, SetsCorrectChargeSpeed)
{
	auto &charge = registry.get<ChargeComponent>(enemy);
	auto &movement = registry.get<MovementComponent>(enemy);
	auto &chase = registry.get<ChaseComponent>(enemy);
	
	charge.chargeDirection = 1.0f;
	charge.chargeTimer.reset();
	charge.chargeTimer.step(std::chrono::duration<float>(0.0f), false);
	
	Systems::ChargeSystem::updateEntity(registry, enemy, 0.1f);
	
	float expectedSpeed = chase.chaseSpeed * charge.chargeSpeedMultiplier;
	EXPECT_FLOAT_EQ(movement.maxSpeedX, expectedSpeed);
}

TEST_F(ChargeSystemTest, StopsChargingWhenPlayerDead)
{
	auto &charge = registry.get<ChargeComponent>(enemy);
	auto &playerState = registry.get<StateComponent>(player);
	
	charge.chargeDirection = -1.0f;
	charge.chargeTimer.reset();
	charge.chargeTimer.step(std::chrono::duration<float>(0.0f), false);
	
	playerState.state = State::Dead;
	
	Systems::ChargeSystem::updateEntity(registry, enemy, 0.1f);
	
	EXPECT_FALSE(charge.chargeTimer.isRunning());
}
