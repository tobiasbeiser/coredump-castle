#include "Engine/ECS/Systems/Enemy/ChaseSystem.hpp"
#include <gtest/gtest.h>

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"

#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/View.hpp"

struct ChaseSystemTest : testing::Test {
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity enemy{};
	GameEngine::ECS::Entity player{};

	void SetUp() override
	{
		using namespace std::chrono_literals;
		enemy = registry.create();
		registry.registerComponent<EnemyTag>(enemy, {});
		registry.registerComponent<InputComponent>(enemy, InputComponent{});
		registry.registerComponent<PositionComponent>(enemy, PositionComponent{.position = {100.0f, 0.0f}});
		registry.registerComponent<ChaseComponent>(enemy,
		                                           ChaseComponent{.chaseSpeed = 120.0f, .reactionDelay = Timer{0.5s}});
		registry.registerComponent<HitBoxComponent>(enemy, {.bodyHitBox = {0.f, 0.f, 20.f, 20.f}});
		registry.registerComponent<CollisionComponent>(enemy, {{0.0f, 0.0f, 20.0f, 20.0f}});
		registry.registerComponent<MovementComponent>(enemy, {.maxSpeedX = 50.0f});
		registry.registerComponent<TransformComponent>(enemy, {.scale = {1.0f, 1.0f}});


		player = registry.create();
		registry.registerComponent<PlayerTag>(player, {});
		registry.registerComponent<PositionComponent>(player, {{0.0f, 0.0f}});
		registry.registerComponent<HurtBoxComponent>(player, {{0.0f, 0.0f, 20.0f, 20.0f}});
		registry.registerComponent<TransformComponent>(player, {});

		const auto floor = registry.create();
		registry.registerComponent<PositionComponent>(floor, {{0.0f, 20.0f}});
		registry.registerComponent<CollisionComponent>(floor, {{-1000.0f, 0.0f, 2000.0f, 20.0f}});

		GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);
	}
};

TEST_F(ChaseSystemTest, MovesRightWhenPlayerIsRight)
{
	auto view = registry.view<InputComponent, MovementComponent, ChaseComponent>();
	auto &enemyInput = view.get<InputComponent>(enemy);
	auto &enemyMovement = view.get<MovementComponent>(enemy);
	auto &enemyChase = view.get<ChaseComponent>(enemy);

	enemyChase.reactionDelay.timeoutNow();

	registry.get<PositionComponent>(player).position = {150.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::updateEntity(registry, enemy, 0.1f);

	EXPECT_FLOAT_EQ(enemyInput.horizontalDirection, 1.0f);
	EXPECT_FLOAT_EQ(enemyMovement.maxSpeedX, enemyChase.chaseSpeed);
}

TEST_F(ChaseSystemTest, MovesLeftWhenPlayerIsLeft)
{
	auto view = registry.view<InputComponent>();
	auto &enemyChase = registry.get<ChaseComponent>(enemy);
	enemyChase.reactionDelay.timeoutNow();
	
	registry.get<PositionComponent>(player).position = {40.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::updateEntity(registry, enemy, 0.1f);

	EXPECT_FLOAT_EQ(view.get<InputComponent>(enemy).horizontalDirection, -1.0f);
}

TEST_F(ChaseSystemTest, WaitsForReactionDelay)
{
	auto view = registry.view<InputComponent>();
	registry.get<PositionComponent>(player).position = {150.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::updateEntity(registry, enemy, 0.1f);
	EXPECT_FLOAT_EQ(view.get<InputComponent>(enemy).horizontalDirection, 0.0f);

	GameEngine::ECS::Systems::ChaseSystem::updateEntity(registry, enemy, 0.5f);
	EXPECT_FLOAT_EQ(view.get<InputComponent>(enemy).horizontalDirection, 1.0f);
}

TEST_F(ChaseSystemTest, StopsWhenEnvironmentBlocked)
{
	auto &enemyChase = registry.get<ChaseComponent>(enemy);
	enemyChase.reactionDelay.timeoutNow();
	
	auto wall = registry.create();
	registry.registerComponent<PositionComponent>(wall, {{120.0f, 0.0f}});
	registry.registerComponent<CollisionComponent>(wall, {{0.0f, -20.0f, 20.0f, 40.0f}}); // Wall covers enemy Y

	GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);

	registry.get<PositionComponent>(player).position = {150.0f, 0.0f};

	GameEngine::ECS::Systems::ChaseSystem::updateEntity(registry, enemy, 0.1f);

	EXPECT_FLOAT_EQ(registry.get<InputComponent>(enemy).horizontalDirection, 0.0f);
}