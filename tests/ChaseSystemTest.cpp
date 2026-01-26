#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
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
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/View.hpp"

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
		registry.registerComponent<VelocityComponent>(enemy, {{0.0f, 0.0f}});

		player = registry.create();
		registry.registerComponent<PlayerTag>(player, {});
		registry.registerComponent<PositionComponent>(player, {{0.0f, 0.0f}});
		registry.registerComponent<CollisionComponent>(player, {{0.0f, 0.0f, 20.0f, 20.0f}});
		registry.registerComponent<VelocityComponent>(player, {{0.0f, 0.0f}});
		registry.registerComponent<TransformComponent>(player, {});
		registry.registerComponent<HurtBoxComponent>(player, {{0.0f, 0.0f, 20.0f, 20.0f}});
		registry.registerComponent<GroundedComponent>(player, {.grounded = true});
		registry.registerComponent<RenderComponent>(player, {});
		registry.registerComponent<StateComponent>(player, {});
	}
};

TEST_F(ChaseSystemTest, IgnoresPlayerWhenOutsidePatrolRange)
{
	auto view = registry.view<InputComponent, MovementComponent, ChaseComponent, PositionComponent>();
	const auto &enemyInput = view.get<InputComponent>(enemy);
	auto &enemyMovement = view.get<MovementComponent>(enemy);
	const auto &enemyChase = view.get<ChaseComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);

	playerPosition.position = {500.0f, 0.0f};
	// set maxspeed to a trap value to see if it gets reseted
	enemyMovement.maxSpeedX = 999.0f;

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.0f);

	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.originalPatrolSpeed);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, IgnoresPlayerWhenVerticalDistanceTooHigh)
{
	auto view = registry.view<InputComponent, MovementComponent, ChaseComponent, PositionComponent>();
	const auto &enemyInput = view.get<InputComponent>(enemy);
	auto &enemyMovement = view.get<MovementComponent>(enemy);
	const auto &enemyChase = view.get<ChaseComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);

	playerPosition.position = {100.0f, 200.0f};

	enemyMovement.maxSpeedX = 999.0f;

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.0f);

	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.originalPatrolSpeed);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, ChasesPlayerRightWhenInPatrolAndOutOfAttackRange)
{
	auto view = registry.view<InputComponent, MovementComponent, ChaseComponent, PositionComponent, RenderComponent>();
	const auto &enemyInput = view.get<InputComponent>(enemy);
	auto &enemyMovement = view.get<MovementComponent>(enemy);
	const auto &enemyChase = view.get<ChaseComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);
	const auto &enemyRender = view.get<RenderComponent>(enemy);

	playerPosition.position = {150.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.0f);
	ASSERT_FALSE(enemyRender.flipX);

	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 1.0f);
	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.chaseSpeed);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, ChasesPlayerLeftWhenInPatrolAndOutOfAttackRange)
{
	auto view = registry.view<InputComponent, MovementComponent, ChaseComponent, PositionComponent, RenderComponent>();
	const auto &enemyInput = view.get<InputComponent>(enemy);
	auto &enemyMovement = view.get<MovementComponent>(enemy);
	const auto &enemyChase = view.get<ChaseComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);
	const auto &enemyRender = view.get<RenderComponent>(enemy);

	playerPosition.position = {40.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	ASSERT_TRUE(enemyRender.flipX);
	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, -1.0f);
	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.chaseSpeed);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, StopsAndAttacksWhenInRange)
{
	auto view = registry.view<InputComponent, PositionComponent>();
	const auto &enemyInput = view.get<InputComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);

	playerPosition.position = {120.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.0f);

	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 0.0f);
	// Attack not pressed yet because of reaction delay
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, AttacksAfterReactionDelay)
{
	auto view = registry.view<InputComponent, PositionComponent>();
	const auto &enemyInput = view.get<InputComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);

	playerPosition.position = {120.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);
	ASSERT_FALSE(enemyInput.attackPressed);

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.5f);

	ASSERT_TRUE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, StopsMovementAndInputWhileAttacking)
{
	auto view = registry.view<InputComponent, PositionComponent>();
	auto &enemyInput = view.get<InputComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);

	playerPosition.position = {120.0f, 0.0f};

	enemyInput.attackPressed = true;
	// trap value
	enemyInput.horizontalDirection = 1.0f;

	auto &attack = registry.view<AttackComponent>().get<AttackComponent>(enemy);
	attack.attackTimer = Timer{std::chrono::duration<float>(1.0f)};
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 0.0f);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, ChargeStartsWhenChanceAndRangeMatch)
{
	using namespace std::chrono_literals;
	registry.registerComponent<ChargeComponent>(enemy, ChargeComponent{});
	auto view = registry.view<InputComponent, MovementComponent, ChaseComponent, PositionComponent>();
	auto &enemyInput = view.get<InputComponent>(enemy);
	auto &enemyMovement = view.get<MovementComponent>(enemy);
	auto &enemyChase = view.get<ChaseComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);

	playerPosition.position = {130.0f, 0.0f};
	enemyChase.reactionDelay.timeoutNow();
	auto &charge = registry.view<ChargeComponent>().get<ChargeComponent>(enemy);
	charge.chargeChance = 1.0f;
	charge.chargeCooldown.timeoutNow();

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	ASSERT_TRUE(charge.chargeTimer.isRunning());
	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 1.0f);
	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.chaseSpeed * charge.chargeSpeedMultiplier);
	ASSERT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, ResetsReactionTimerWhenPlayerLeavesRange)
{
	auto view = registry.view<InputComponent, PositionComponent>();
	auto &enemyInput = view.get<InputComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);

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
	auto view = registry.view<InputComponent, MovementComponent, ChaseComponent, PositionComponent, GroundedComponent,
	                          RenderComponent>();
	const auto &enemyInput = view.get<InputComponent>(enemy);
	const auto &enemyMovement = view.get<MovementComponent>(enemy);
	const auto &enemyChase = view.get<ChaseComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);
	auto &playerGrounded = view.get<GroundedComponent>(player);
	const auto &enemyRender = view.get<RenderComponent>(enemy);

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
	auto enemyView = registry.view<InputComponent, ChaseComponent>();
	auto playerView = registry.view<PositionComponent>();
	auto &enemyChase = enemyView.get<ChaseComponent>(enemy);
	enemyChase.detectionWidthRange = 150.0f;
	enemyChase.detectionHeightRange = 100.0f;

	auto &playerPosition = playerView.get<PositionComponent>(player);
	playerPosition.position = {150.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.6f);

	const auto &enemyInput = enemyView.get<InputComponent>(enemy);
	EXPECT_FLOAT_EQ(enemyInput.horizontalDirection, 0.0f);
	EXPECT_TRUE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, RangedEnemyDoesNotAttackWhenLineOfSightBlocked)
{
	registry.registerComponent<RangedAttackComponent>(enemy, {});
	auto enemyView = registry.view<InputComponent, ChaseComponent>();
	auto playerView = registry.view<PositionComponent>();
	auto &enemyChase = enemyView.get<ChaseComponent>(enemy);
	enemyChase.detectionWidthRange = 200.0f;
	enemyChase.detectionHeightRange = 50.0f;

	const auto blocker = registry.create();
	registry.registerComponent<PositionComponent>(blocker, {{120.0f, 0.0f}});
	registry.registerComponent<CollisionComponent>(blocker, {{0.0f, -5.0f, 10.0f, 20.0f}});
	GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);

	auto &playerPosition = playerView.get<PositionComponent>(player);
	playerPosition.position = {150.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.6f);

	const auto &enemyInput = enemyView.get<InputComponent>(enemy);
	EXPECT_FALSE(enemyInput.attackPressed);
}

TEST_F(ChaseSystemTest, ChargeStopsOnContact)
{
	using namespace std::chrono_literals;
	registry.registerComponent<ChargeComponent>(enemy, ChargeComponent{});
	auto view = registry.view<InputComponent, MovementComponent, ChaseComponent, PositionComponent>();
	auto &enemyInput = view.get<InputComponent>(enemy);
	auto &enemyChase = view.get<ChaseComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);

	// Start charge
	playerPosition.position = {130.0f, 0.0f};
	enemyChase.reactionDelay.timeoutNow();
	auto &charge = registry.view<ChargeComponent>().get<ChargeComponent>(enemy);
	charge.chargeChance = 1.0f;
	charge.chargeCooldown.timeoutNow();

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);
	ASSERT_TRUE(charge.chargeTimer.isRunning());
	ASSERT_NE(enemyInput.horizontalDirection, 0.0f);

	// Move enemy to overlap with player
	auto &enemyPosition = view.get<PositionComponent>(enemy);
	enemyPosition.position = playerPosition.position; // Perfect overlap

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	// Charge should have stopped
	ASSERT_FALSE(charge.chargeTimer.isRunning());
	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 0.0f);
}

TEST_F(ChaseSystemTest, ChargeSnapsToCorrectSideWhenOvershooting)
{
	using namespace std::chrono_literals;
	registry.registerComponent<ChargeComponent>(enemy, ChargeComponent{});
	auto view = registry.view<InputComponent, MovementComponent, ChaseComponent, PositionComponent, VelocityComponent>();
	auto &enemyInput = view.get<InputComponent>(enemy);
	auto &enemyChase = view.get<ChaseComponent>(enemy);
	auto &enemyVel = view.get<VelocityComponent>(enemy);
	auto &playerPosition = view.get<PositionComponent>(player);

	// Start charge Left -> Right
	playerPosition.position = {130.0f, 0.0f}; // Player Box: [130, 150], Center 140
	enemyChase.reactionDelay.timeoutNow();
	auto &charge = registry.view<ChargeComponent>().get<ChargeComponent>(enemy);
	charge.chargeChance = 1.0f;
	charge.chargeCooldown.timeoutNow();

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);
	ASSERT_TRUE(charge.chargeTimer.isRunning());
	// Moving Right
	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 1.0f);
	enemyVel.velocity.x = 100.0f; 

	// Overshoot: Move Enemy to [145, 165]. Center 155.
	// Player Center is 140. Enemy > Player. directionToPlayer will be -1 (Left).
	// Overlap is [145, 150] (5 pixels).
	auto &enemyPosition = view.get<PositionComponent>(enemy);
	enemyPosition.position = {145.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	// Should stop
	ASSERT_FALSE(charge.chargeTimer.isRunning());
	
	// Should snap to LEFT of player (130 - 20 = 110).
	// Because we were charging Right.
	// Current BUG: It checks directionToPlayer (-1), thinks we charge Left, so snaps to Right of Player (130+20=150).
	// So if it equals 150, it passed through.
	// If it equals 110, it blocked correctly.
	
	ASSERT_NEAR(enemyPosition.position.x, 110.0f, 0.1f);
}

TEST_F(ChaseSystemTest, RangedEnemyResetsWhenPlayerOutOfRange)
{
	registry.registerComponent<RangedAttackComponent>(enemy, {});
	auto enemyView = registry.view<InputComponent, ChaseComponent>();
	auto playerView = registry.view<PositionComponent>();
	auto &enemyChase = enemyView.get<ChaseComponent>(enemy);
	enemyChase.detectionWidthRange = 100.0f;
	enemyChase.detectionHeightRange = 50.0f;

	auto &playerPosition = playerView.get<PositionComponent>(player);
	playerPosition.position = {300.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	const auto &enemyInput = enemyView.get<InputComponent>(enemy);
	EXPECT_FALSE(enemyInput.attackPressed);
}