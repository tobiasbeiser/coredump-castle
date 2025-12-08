#include "Engine/ECS/Systems/Combat/EnemyAISystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/GameEngine.hpp"
#include <gtest/gtest.h>

class EnemyAISystemTest : public ::testing::Test {
  protected:
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity enemy{};
	GameEngine::ECS::Entity player{};

	void SetUp() override
	{
		enemy = registry.create();
		registry.registerComponent<EnemyTag>(enemy, {});
		registry.registerComponent<PositionComponent>(enemy, PositionComponent{{100.f, 0.f}, {100.f, 0.f}});
		registry.registerComponent<StateComponent>(enemy, {State::Idle});
		registry.registerComponent<AttackComponent>(enemy, {{1.0f}, {0.5f}});
		registry.registerComponent<HitBoxComponent>(enemy, HitBoxComponent{{0.0f, 0.0f, 50.0f, 50.0f}});

		player = registry.create();
		registry.registerComponent<PlayerTag>(player, {});
		registry.registerComponent<StateComponent>(player, {State::Idle});
		registry.registerComponent<PositionComponent>(player, PositionComponent{{200.0f, 200.0f}});
		registry.registerComponent<BlinkComponent>(player, BlinkComponent(0.5f));
		registry.registerComponent<HitBoxComponent>(player, {{0.0f, 0.0f, 50.0f, 50.0f}});
	}
};

TEST_F(EnemyAISystemTest, SwitchesToAttackWhenPlayerIsInRange)
{
	auto &[position, previousPosition] = registry.get<PositionComponent>(player);
	position.x = 110.0f;
	position.y = 0.0f;

	GameEngine::ECS::Systems::EnemyAISystem::update(registry, 0.1f);

	const auto &[state] = registry.get<StateComponent>(enemy);
	EXPECT_EQ(state, State::Attacking);
}

TEST_F(EnemyAISystemTest, RemainsIdleWhenPlayerIsOutOfRange)
{
	GameEngine::ECS::Systems::EnemyAISystem::update(registry, 0.1f);

	const auto &[state] = registry.get<StateComponent>(enemy);
	EXPECT_EQ(state, State::Idle);
}