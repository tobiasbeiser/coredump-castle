#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/MeleeAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/AiStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/VisionComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Enemy/AiDecisionSystem.hpp"
#include <gtest/gtest.h>

struct AiDecisionSystemTests : testing::Test {
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity enemy{};
	GameEngine::ECS::Entity player{};

	void SetUp() override
	{
		enemy = registry.create();
		registry.registerComponent<PositionComponent>(enemy, {{0.0f, 0.0f}});
		registry.registerComponent<HitBoxComponent>(enemy, {.bodyHitBox = {0, 0, 20, 20}});
		registry.registerComponent<AiStateComponent>(enemy, {.state = AiState::Idle});
		registry.registerComponent<VisionComponent>(enemy, {.visionRange = 200.0f});
		registry.registerComponent<ChaseComponent>(enemy, {});
		registry.registerComponent<MeleeAttackComponent>(enemy, {.attackRange = 50.0f});

		player = registry.create();
		registry.registerComponent<PlayerTag>(player, {});
		registry.registerComponent<PositionComponent>(player, {{0.0f, 0.0f}});
		registry.registerComponent<HurtBoxComponent>(player, {{0, 0, 20, 20}});
		registry.registerComponent<TransformComponent>(player, {});

		GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);
	}
};

TEST_F(AiDecisionSystemTests, SwitchesToChaseWhenSeen)
{
	registry.get<PositionComponent>(player).position = {100.0f, 0.0f};

	GameEngine::ECS::Systems::AiDecisionSystem::update(registry);

	EXPECT_EQ(registry.get<AiStateComponent>(enemy).state, AiState::Chase);
}

TEST_F(AiDecisionSystemTests, RemainsIdleWhenOutOfVision)
{
	registry.get<PositionComponent>(player).position = {300.0f, 0.0f};

	GameEngine::ECS::Systems::AiDecisionSystem::update(registry);

	EXPECT_EQ(registry.get<AiStateComponent>(enemy).state, AiState::Idle);
}

TEST_F(AiDecisionSystemTests, SwitchesToAttackWhenInMeleeRange)
{
	registry.get<PositionComponent>(player).position = {40.0f, 0.0f};

	GameEngine::ECS::Systems::AiDecisionSystem::update(registry);

	EXPECT_EQ(registry.get<AiStateComponent>(enemy).state, AiState::Attack);
}

TEST_F(AiDecisionSystemTests, DoesNotSeeThroughWalls)
{
	auto wall = registry.create();
	registry.registerComponent<PositionComponent>(wall, {{50.0f, 0.0f}});
	registry.registerComponent<CollisionComponent>(wall, {{0, 0, 20, 100}}); // Tall wall
	GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);

	registry.get<PositionComponent>(player).position = {100.0f, 0.0f};

	GameEngine::ECS::Systems::AiDecisionSystem::update(registry);

	EXPECT_NE(registry.get<AiStateComponent>(enemy).state, AiState::Chase);
}