#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
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
#include "Engine/ECS/View.hpp"

#include <gtest/gtest.h>

class ChaseSystemTest : public ::testing::Test {
  protected:
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity enemy{};
	GameEngine::ECS::Entity player{};

	void SetUp() override
	{
		using namespace std::chrono_literals;
		enemy = registry.create();
		registry.registerComponent<EnemyTag>(enemy, {});
		registry.registerComponent<InputComponent>(enemy, InputComponent{});
		registry.registerComponent<PatrolComponent>(enemy, PatrolComponent{{0.0f, 0.0f}, {200.0f, 0.0f}});
		registry.registerComponent<PositionComponent>(enemy, {{100.0f, 0.0f}});
		registry.registerComponent<ChaseComponent>(enemy, ChaseComponent{.verticalDetectionHeight = 50.0f,
		                                                                 .chaseSpeed = 120.0f,
		                                                                 .originalPatrolSpeed = 50.0f,
		                                                                 .reactionDelay = Timer{0.5s}});
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
		registry.registerComponent<RenderComponent>(player, {});
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

	GameEngine::ECS::Systems::ChaseSystem::update(registry, 0.1f);

	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 0.0f);
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

	// Should still chase
	ASSERT_FALSE(enemyRender.flipX);
	ASSERT_FLOAT_EQ(enemyInput.horizontalDirection, 1.0f);
	ASSERT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.chaseSpeed);
}