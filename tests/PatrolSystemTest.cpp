#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include <gtest/gtest.h>

class PatrolSystemTest : public ::testing::Test {
  protected:
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity enemy{};

	void SetUp() override
	{
		enemy = registry.create();
		registry.registerComponent<EnemyTag>(enemy, {});
		registry.registerComponent<PositionComponent>(enemy, {{100.0f, 0.0f}});
		registry.registerComponent<VelocityComponent>(enemy, {{0.0f, 0.0f}});
		registry.registerComponent<RenderComponent>(enemy, {});
		registry.registerComponent<CollisionComponent>(enemy, {0.0f, 0.0f, 10.0f, 10.0f});
		registry.registerComponent<PatrolComponent>(enemy, PatrolComponent{{0.0f, 0.0f}, {200.0f, 0.0f}, 50.0f, 1});
	}
};

TEST_F(PatrolSystemTest, MovesAccordingToDirection)
{
	GameEngine::ECS::Systems::PatrolSystem::update(registry);
	const auto &[velocity] = registry.get<VelocityComponent>(enemy);

	EXPECT_FLOAT_EQ(velocity.x, 50.0f);
}

TEST_F(PatrolSystemTest, TurnsAroundAtMaxBounds)
{
	auto &[position, previousPosition] = registry.get<PositionComponent>(enemy);
	const auto &patrol = registry.get<PatrolComponent>(enemy);
	const auto &render = registry.get<RenderComponent>(enemy);

	position.x = 191.0f;

	GameEngine::ECS::Systems::PatrolSystem::update(registry);

	EXPECT_EQ(patrol.direction, -1);
	EXPECT_FALSE(render.flipX);
	EXPECT_LT(position.x, 191.0f);
}

TEST_F(PatrolSystemTest, TurnsAroundAtMinBounds)
{
	auto &[position, previousPosition] = registry.get<PositionComponent>(enemy);
	auto &patrol = registry.get<PatrolComponent>(enemy);
	const auto &render = registry.get<RenderComponent>(enemy);

	patrol.direction = -1;
	position.x = -1.0f;

	GameEngine::ECS::Systems::PatrolSystem::update(registry);

	EXPECT_EQ(patrol.direction, 1);
	EXPECT_TRUE(render.flipX);
	EXPECT_GT(position.x, -1.0f);
}