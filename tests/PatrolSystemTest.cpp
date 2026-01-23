#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Entity.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"
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
		registry.registerComponent<InputComponent>(enemy, {});
		registry.registerComponent<RenderComponent>(enemy, {});
		registry.registerComponent<CollisionComponent>(enemy, {{0.0f, 0.0f, 10.0f, 10.0f}});
		registry.registerComponent<PatrolComponent>(enemy, PatrolComponent{{0.0f, 0.0f}, {200.0f, 0.0f}});
	}
};

TEST_F(PatrolSystemTest, StartsMovingRightWhenIdle)
{
	auto &input = registry.get<InputComponent>(enemy);

	GameEngine::ECS::Systems::PatrolSystem::update(registry);

	EXPECT_FLOAT_EQ(input.horizontalDirection, 1.0f);
}

TEST_F(PatrolSystemTest, ReversesDirectionAtMaxBounds)
{
	auto &[position, previousPosition] = registry.get<PositionComponent>(enemy);
	auto &input = registry.get<InputComponent>(enemy);

	position.x = 191.0f;
	input.horizontalDirection = 1.0f;

	GameEngine::ECS::Systems::PatrolSystem::update(registry);

	EXPECT_FLOAT_EQ(input.horizontalDirection, -1.0f);
}

TEST_F(PatrolSystemTest, ReversesDirectionAtMinBounds)
{
	auto &[position, previousPosition] = registry.get<PositionComponent>(enemy);
	auto &input = registry.get<InputComponent>(enemy);

	position.x = -1.0f;
	input.horizontalDirection = -1.0f;

	GameEngine::ECS::Systems::PatrolSystem::update(registry);

	EXPECT_FLOAT_EQ(input.horizontalDirection, 1.0f);
}