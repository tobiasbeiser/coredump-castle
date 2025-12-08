#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Systems/Combat/EnemyAISystem.hpp"
#include "Engine/GameEngine.hpp"
#include <Engine/ECS/Components/Core/VelocityComponent.hpp>
#include <Engine/ECS/Components/Gameplay/Combat/PushbackComponent.hpp>
#include <Engine/ECS/Systems/Combat/PushbackSystem.hpp>
#include <gtest/gtest.h>

class PushbackSystemTest : public ::testing::Test {
  protected:
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity player{};

	void SetUp() override
	{
		player = registry.create();
		registry.registerComponent<PlayerTag>(player, {});
		registry.registerComponent<PositionComponent>(player, {{0.0f, 0.0f}});
		registry.registerComponent<VelocityComponent>(player, {{0.0f, 0.0f}});
		registry.registerComponent<BlinkComponent>(player, BlinkComponent(0.5f));
		registry.registerComponent<HitBoxComponent>(player, HitBoxComponent{{0, 0, 50, 50}});
	}

	GameEngine::ECS::Entity createEnemy(float x, float y, float pushbackForce = -500.0f)
	{
		auto enemy = registry.create();
		registry.registerComponent<EnemyTag>(enemy, {});
		registry.registerComponent<PositionComponent>(enemy, {{x, y}, {0.0f, 0.0f}});
		registry.registerComponent<VelocityComponent>(enemy, {{10.0f, 0.0f}});
		registry.registerComponent<PushbackComponent>(enemy, {pushbackForce});
		registry.registerComponent<HitBoxComponent>(enemy, HitBoxComponent{{0, 0, 50, 50}});
		return enemy;
	}

	GameEngine::ECS::Entity createSpike(float x, float y)
	{
		auto spike = registry.create();
		registry.registerComponent<SpikeTag>(spike, {});
		registry.registerComponent<PositionComponent>(spike, {{x, y}, {0.0f, 0.0f}});
		registry.registerComponent<HitBoxComponent>(spike, HitBoxComponent{{0, 0, 50, 50}});
		return spike;
	}
};

TEST_F(PushbackSystemTest, AppliesPushbackOnCollision)
{
	auto enemy = createEnemy(40.0f, 0.0f);

	GameEngine::ECS::Systems::PushbackSystem::update(registry);

	const auto &[playerVelocity] = registry.get<VelocityComponent>(player);
	const auto &[enemyVelocity] = registry.get<VelocityComponent>(enemy);
	const auto &blink = registry.get<BlinkComponent>(player);

	EXPECT_FLOAT_EQ(playerVelocity.y, -500.0f);
	EXPECT_FLOAT_EQ(enemyVelocity.x, 0.0f);
	EXPECT_FLOAT_EQ(enemyVelocity.y, 0.0f);
	EXPECT_TRUE(!blink.timer.isTimeout());
}

TEST_F(PushbackSystemTest, NoPushbackWithoutCollision)
{
	createEnemy(200.0f, 0.0f);

	GameEngine::ECS::Systems::PushbackSystem::update(registry);

	const auto &[velocity] = registry.get<VelocityComponent>(player);
	EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}

TEST_F(PushbackSystemTest, SpikeCollisionAppliesPushback)
{
	registry.registerComponent<PushbackComponent>(player, {-400.0f});
	createSpike(0.0f, 0.0f);

	GameEngine::ECS::Systems::PushbackSystem::update(registry);

	const auto &[velocity] = registry.get<VelocityComponent>(player);
	const auto &blink = registry.get<BlinkComponent>(player);

	EXPECT_FLOAT_EQ(velocity.y, -400.0f);
	EXPECT_TRUE(!blink.timer.isTimeout());
}

TEST_F(PushbackSystemTest, NoSpikeCollisionNoPushback)
{
	registry.registerComponent<PushbackComponent>(player, {-400.0f});
	createSpike(200.0f, 0.0f);

	GameEngine::ECS::Systems::PushbackSystem::update(registry);

	const auto &[velocity] = registry.get<VelocityComponent>(player);
	EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}

TEST_F(PushbackSystemTest, EnemyVelocityResetOnCollision)
{
	const auto enemy = createEnemy(40.0f, 0.0f);

	GameEngine::ECS::Systems::PushbackSystem::update(registry);

	const auto &[velocity] = registry.get<VelocityComponent>(enemy);
	EXPECT_FLOAT_EQ(velocity.x, 0.0f);
	EXPECT_FLOAT_EQ(velocity.y, 0.0f);
}