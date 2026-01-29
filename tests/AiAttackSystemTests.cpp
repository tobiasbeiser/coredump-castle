#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/AiStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Enemy/AiAttackSystem.hpp"
#include <gtest/gtest.h>

struct AiAttackSystemTests : testing::Test {
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity enemy{};
	GameEngine::ECS::Entity player{};

	void SetUp() override
	{
		enemy = registry.create();
		registry.registerComponent<AiStateComponent>(enemy, {.state = AiState::Attack}); // Force Attack State
		registry.registerComponent<InputComponent>(enemy, {});
		registry.registerComponent<PositionComponent>(enemy, {{0.0f, 0.0f}});
		registry.registerComponent<MovementComponent>(enemy, MovementComponent{.maxSpeedX = 100.0f});
		registry.registerComponent<RenderComponent>(enemy, {});
		registry.registerComponent<AttackComponent>(enemy, {});
		registry.registerComponent<HitBoxComponent>(enemy, {.bodyHitBox = {0, 0, 20, 20}});

		player = registry.create();
		registry.registerComponent<PlayerTag>(player, {});
		registry.registerComponent<PositionComponent>(player, {{100.0f, 0.0f}});
		registry.registerComponent<HurtBoxComponent>(player, {{0, 0, 20, 20}});
		registry.registerComponent<TransformComponent>(player, {});
	}
};

TEST_F(AiAttackSystemTests, StopsMovementWhenAttacking)
{
	GameEngine::ECS::Systems::AiAttackSystem::update(registry, 0.1f);
	EXPECT_FLOAT_EQ(registry.get<MovementComponent>(enemy).maxSpeedX, 0.0f);
}

TEST_F(AiAttackSystemTests, FacesPlayerWhenAttacking)
{
	GameEngine::ECS::Systems::AiAttackSystem::update(registry, 0.1f);
	EXPECT_FALSE(registry.get<RenderComponent>(enemy).isFacingLeft());

	registry.get<PositionComponent>(player).position = {-100.0f, 0.0f};
	GameEngine::ECS::Systems::AiAttackSystem::update(registry, 0.1f);
	EXPECT_TRUE(registry.get<RenderComponent>(enemy).isFacingLeft());
}

TEST_F(AiAttackSystemTests, PressesAttackButtonForMelee)
{
	GameEngine::ECS::Systems::AiAttackSystem::update(registry, 0.1f);

	EXPECT_TRUE(registry.get<InputComponent>(enemy).attackPressed);
}

TEST_F(AiAttackSystemTests, RangedAttackSetsSnapshot)
{
	registry.registerComponent<RangedAttackComponent>(enemy, {});

	glm::vec2 expectedTarget = {110.0f, 10.0f};

	GameEngine::ECS::Systems::AiAttackSystem::update(registry, 0.1f);

	auto &ranged = registry.get<RangedAttackComponent>(enemy);
	EXPECT_TRUE(registry.get<InputComponent>(enemy).attackPressed);
	EXPECT_NEAR(ranged.targetSnapshot.x, expectedTarget.x, 1.0f);
}

TEST_F(AiAttackSystemTests, RangedAttackBlockedByWall)
{
	registry.registerComponent<RangedAttackComponent>(enemy, {});

	// Wall in between
	const auto wall = registry.create();
	registry.registerComponent<PositionComponent>(wall, {{50.0f, 0.0f}});
	registry.registerComponent<CollisionComponent>(wall, {{0, 0, 20, 50}});
	GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);

	GameEngine::ECS::Systems::AiAttackSystem::update(registry, 0.1f);

	EXPECT_FALSE(registry.get<InputComponent>(enemy).attackPressed);
}