#include "Engine/ECS/Systems/BlinkSystem.hpp"
#include <gtest/gtest.h>

class BlinkSystemTest : public ::testing::Test {
  protected:
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity entity;

	void SetUp() override
	{
		entity = registry.create();
		registry.registerComponent<BlinkComponent>(entity, BlinkComponent(1.0f));
	}
};

TEST_F(BlinkSystemTest, TimerDecreasesOnUpdate)
{
	auto &blink = registry.get<BlinkComponent>(entity);
	blink.timer.reset();

	const float initialTime = blink.timer.getTime();

	GameEngine::ECS::Systems::BlinkSystem::update(registry, 0.1f);

	EXPECT_LT(initialTime, blink.timer.getTime());
	EXPECT_FLOAT_EQ(blink.timer.getTime(), initialTime + 0.1f);
}

TEST_F(BlinkSystemTest, UpdateHandlesMultipleEntities)
{
	auto entity2 = registry.create();
	registry.registerComponent<BlinkComponent>(entity2, BlinkComponent(2.0f));

	auto &blink1 = registry.get<BlinkComponent>(entity);
	auto &blink2 = registry.get<BlinkComponent>(entity2);
	blink1.timer.reset();
	blink2.timer.reset();

	GameEngine::ECS::Systems::BlinkSystem::update(registry, 0.5f);

	EXPECT_FLOAT_EQ(blink1.timer.getTime(), 0.5f);
	EXPECT_FLOAT_EQ(blink2.timer.getTime(), 0.5f);
}

TEST_F(BlinkSystemTest, TimerReachesTimeout)
{
	auto &blink = registry.get<BlinkComponent>(entity);
	blink.timer.reset();

	GameEngine::ECS::Systems::BlinkSystem::update(registry, 1.5f);

	EXPECT_TRUE(blink.timer.isTimeout());
}