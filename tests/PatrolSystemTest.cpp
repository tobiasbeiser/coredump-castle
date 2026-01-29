#include "Engine/ECS/Systems/Enemy/PatrolSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/AiStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/View.hpp"

#include <gtest/gtest.h>

struct PatrolSystemTest : testing::Test {
	GameEngine::ECS::Registry registry{};
	GameEngine::ECS::Entity enemy{};

	void SetUp() override
	{
		enemy = registry.create();
		registry.registerComponent<EnemyTag>(enemy, {});
		registry.registerComponent<PositionComponent>(enemy, {{100.0f, 0.0f}});
		registry.registerComponent<InputComponent>(enemy, {});
		registry.registerComponent<RenderComponent>(enemy, {});
		registry.registerComponent<CollisionComponent>(enemy, {{0.0f, 0.0f, 10.0f, 10.0f}});
		registry.registerComponent<PatrolComponent>(enemy, PatrolComponent{});
		registry.registerComponent<AiStateComponent>(enemy, AiStateComponent{AiState::Patrol});
		registry.registerComponent<MovementComponent>(enemy, {.accelerationRate = 500.0f, .maxSpeedX = 100.0f});

		auto collider = registry.create();
		registry.registerComponent<CollisionComponent>(collider, {{0.0f, 0.0f, 100.0f, 10.0f}});
		registry.registerComponent<PositionComponent>(collider, {.position = {10.0f, -10.0f}}); // Ground collider

		GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);
	}
};

TEST_F(PatrolSystemTest, StartsMovingLeftWhenIdle)
{
	auto view = registry.view<InputComponent>();
	auto &input = view.get<InputComponent>(enemy);

	GameEngine::ECS::Systems::PatrolSystem::update(registry);

	EXPECT_FLOAT_EQ(input.horizontalDirection, -1.0f);
}

TEST_F(PatrolSystemTest, ReversesDirectionAtMaxBounds)
{
	auto view = registry.view<PositionComponent, InputComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(enemy);
	auto &input = view.get<InputComponent>(enemy);

	position.x = 191.0f;
	input.horizontalDirection = 1.0f;

	GameEngine::ECS::Systems::PatrolSystem::update(registry);

	EXPECT_FLOAT_EQ(input.horizontalDirection, -1.0f);
}

TEST_F(PatrolSystemTest, ReversesDirectionAtMinBounds)
{
	auto view = registry.view<PositionComponent, InputComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(enemy);
	auto &input = view.get<InputComponent>(enemy);

	position.x = -1.0f;
	input.horizontalDirection = -1.0f;

	GameEngine::ECS::Systems::PatrolSystem::update(registry);

	EXPECT_FLOAT_EQ(input.horizontalDirection, 1.0f);
}