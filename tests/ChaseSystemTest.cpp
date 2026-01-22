#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Entity.hpp"
#include "Engine/ECS/Registry.hpp"

#include <gtest/gtest.h>

class ChaseSystemTest : public ::testing::Test {
  protected:
	// NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
	GameEngine::ECS::Registry registry;
	// NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
	GameEngine::ECS::Entity enemy{};
	// NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
	GameEngine::ECS::Entity player{};

	void SetUp() override
	{
		using namespace std::chrono_literals;
		enemy = registry.create();
		registry.registerComponent<EnemyTag>(enemy, {});
		registry.registerComponent<InputComponent>(enemy, InputComponent{});
		registry.registerComponent<PatrolComponent>(enemy, PatrolComponent{{0.0f, 0.0f}, {200.0f, 0.0f}});
		registry.registerComponent<PositionComponent>(enemy, {{100.0f, 0.0f}});
		registry.registerComponent<ChaseComponent>(enemy, ChaseComponent{.chaseSpeed = 120.0f,
		                                                                 .originalPatrolSpeed = 50.0f,
		                                                                 .reactionDelay = Timer{0.5s},
		                                                                 .detectionHeightRange = 50.0f});
		registry.registerComponent<HitBoxComponent>(
		    enemy, {.bodyHitBox = {0.f, 0.f, 20.f, 20.f}, .weaponHitBox = {0.f, 0.f, 0.f, 0.f}});
		registry.registerComponent<HurtBoxComponent>(enemy, {.hurtBox = {0.f, 0.f, 20.f, 20.f}});
		registry.registerComponent<CollisionComponent>(enemy, {{0.0f, 0.0f, 20.0f, 20.0f}});
		registry.registerComponent<MovementComponent>(enemy, {.maxSpeedX = 50.0f});
		registry.registerComponent<GroundedComponent>(enemy, {});
		registry.registerComponent<AttackComponent>(enemy, {});
		registry.registerComponent<RenderComponent>(enemy, {});
		registry.registerComponent<TransformComponent>(enemy, {.scale = {1.0f, 1.0f}});

		player = registry.create();
		registry.registerComponent<PlayerTag>(player, {});
		registry.registerComponent<PositionComponent>(player, {{0.0f, 0.0f}});
		registry.registerComponent<CollisionComponent>(player, {{0.0f, 0.0f, 20.0f, 20.0f}});
		registry.registerComponent<VelocityComponent>(player, {{0.0f, 0.0f}});
		registry.registerComponent<TransformComponent>(player, {});
		registry.registerComponent<HurtBoxComponent>(player, {{0.0f, 0.0f, 20.0f, 20.0f}});
		registry.registerComponent<GroundedComponent>(player, {.grounded = true});
	}
};

TEST_F(ChaseSystemTest, IgnoresPlayerWhenOutsidePatrolRange)
{
	const auto &enemyInput = registry.get<InputComponent>(enemy);
	auto &enemyMovement = registry.get<MovementComponent>(enemy);
	const auto &enemyChase = registry.get<ChaseComponent>(enemy);
	auto &playerPosition = registry.get<PositionComponent>(player);

	playerPosition.position = {500.0f, 0.0f};
	// set maxspeed to a trap value to see if it gets reseted
	enemyMovement.maxSpeedX = 999.0f;

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.0f);

	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.originalPatrolSpeed);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, IgnoresPlayerWhenVerticalDistanceTooHigh)
{
	const auto &enemyInput = registry.get<InputComponent>(enemy);
	auto &enemyMovement = registry.get<MovementComponent>(enemy);
	const auto &enemyChase = registry.get<ChaseComponent>(enemy);
	auto &playerPosition = registry.get<PositionComponent>(player);

	playerPosition.position = {100.0f, 200.0f};

	enemyMovement.maxSpeedX = 999.0f;

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.0f);

	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.originalPatrolSpeed);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, ChasesPlayerRightWhenInPatrolAndOutOfAttackRange)
{
	auto &enemyInput = registry.get<InputComponent>(enemy);
	const auto &enemyMovement = registry.get<MovementComponent>(enemy);
	const auto &enemyChase = registry.get<ChaseComponent>(enemy);
	auto &playerPosition = registry.get<PositionComponent>(player);
	const auto &enemyRender = registry.get<RenderComponent>(enemy);

	playerPosition.position = {150.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.0f);
	ASSERT_FALSE(enemyRender.flipX);

	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 1.0f);
	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.chaseSpeed);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, ChasesPlayerLeftWhenInPatrolAndOutOfAttackRange)
{
	auto &enemyInput = registry.get<InputComponent>(enemy);
	const auto &enemyMovement = registry.get<MovementComponent>(enemy);
	const auto &enemyChase = registry.get<ChaseComponent>(enemy);
	auto &playerPosition = registry.get<PositionComponent>(player);
	const auto &enemyRender = registry.get<RenderComponent>(enemy);

	playerPosition.position = {40.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	ASSERT_TRUE(enemyRender.flipX);
	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, -1.0f);
	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.chaseSpeed);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, StopsAndAttacksWhenInRange)
{
	const auto &enemyInput = registry.get<InputComponent>(enemy);
	const auto &enemyMovement = registry.get<MovementComponent>(enemy);
	auto &playerPosition = registry.get<PositionComponent>(player);

	playerPosition.position = {120.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.0f);

	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 0.0f);
	// Attack not pressed yet because of reaction delay
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, AttacksAfterReactionDelay)
{
	const auto &enemyInput = registry.get<InputComponent>(enemy);
	auto &playerPosition = registry.get<PositionComponent>(player);

	playerPosition.position = {120.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);
	ASSERT_FALSE(enemyInput.attackPressed);

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.5f);

	ASSERT_TRUE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, StopsMovementAndInputWhileAttacking)
{
	auto &enemyInput = registry.get<InputComponent>(enemy);
	auto &playerPosition = registry.get<PositionComponent>(player);

	playerPosition.position = {120.0f, 0.0f};

	enemyInput.attackPressed = true;
	// trap value
	enemyInput.horizontalDirection = 1.0f;

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 0.0f);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, ResetsReactionTimerWhenPlayerLeavesRange)
{
	const auto &enemyInput = registry.get<InputComponent>(enemy);
	auto &playerPosition = registry.get<PositionComponent>(player);

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.3f);
	ASSERT_FALSE(enemyInput.attackPressed);

	playerPosition.position = {400.0f, 0.0f};
	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	playerPosition.position = {120.0f, 0.0f};

	// If reset() fails the total time would be 0.3 + 0.3 = 0.6 (> 0.5), leading so to Attack = TRUE.
	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	ASSERT_FALSE(enemyInput.attackPressed);

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.5f);

	ASSERT_TRUE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, ChasesPlayerWhenJumpingHigh)
{
	const auto &enemyInput = registry.get<InputComponent>(enemy);
	const auto &enemyMovement = registry.get<MovementComponent>(enemy);
	const auto &enemyChase = registry.get<ChaseComponent>(enemy);
	auto &playerPosition = registry.get<PositionComponent>(player);
	auto &playerGrounded = registry.get<GroundedComponent>(player);
	const auto &enemyRender = registry.get<RenderComponent>(enemy);

	playerGrounded.grounded = false;

	playerPosition.position = {150.0f, 100.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.0f);

	ASSERT_FALSE(enemyRender.flipX);
	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 1.0f);
	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.chaseSpeed);
}

TEST_F(ChaseSystemTest, RangedEnemyStopsAndAttacksInDetectionRange)
{
	using namespace std::chrono_literals;
	registry.registerComponent<RangedAttackComponent>(enemy, {});
	auto &enemyChase = registry.get<ChaseComponent>(enemy);
	enemyChase.detectionWidthRange = 150.0f;
	enemyChase.detectionHeightRange = 100.0f;

	auto &playerPosition = registry.get<PositionComponent>(player);
	playerPosition.position = {150.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.6f);

	const auto &enemyInput = registry.get<InputComponent>(enemy);
	EXPECT_FLOAT_EQ(enemyInput.horizontalDirection, 0.0f);
	EXPECT_TRUE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, RangedEnemyDoesNotAttackWhenLineOfSightBlocked)
{
	registry.registerComponent<RangedAttackComponent>(enemy, {});
	auto &enemyChase = registry.get<ChaseComponent>(enemy);
	enemyChase.detectionWidthRange = 200.0f;
	enemyChase.detectionHeightRange = 50.0f;

	const auto blocker = registry.create();
	registry.registerComponent<PositionComponent>(blocker, {{120.0f, 0.0f}});
	registry.registerComponent<CollisionComponent>(blocker, {{0.0f, -5.0f, 10.0f, 20.0f}});

	auto &playerPosition = registry.get<PositionComponent>(player);
	playerPosition.position = {150.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.6f);

	const auto &enemyInput = registry.get<InputComponent>(enemy);
	EXPECT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, RangedEnemyResetsWhenPlayerOutOfRange)
{
	registry.registerComponent<RangedAttackComponent>(enemy, {});
	auto &enemyChase = registry.get<ChaseComponent>(enemy);
	enemyChase.detectionWidthRange = 100.0f;
	enemyChase.detectionHeightRange = 50.0f;

	auto &playerPosition = registry.get<PositionComponent>(player);
	playerPosition.position = {300.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	const auto &enemyInput = registry.get<InputComponent>(enemy);
	EXPECT_FALSE(enemyInput.attackPressed);
}