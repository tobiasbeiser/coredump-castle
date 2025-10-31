#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/PhysicsSystem.hpp"
#include <gtest/gtest.h>

using namespace GameEngine::ECS;
using namespace GameEngine::ECS::Systems;

class PhysicsSystemTest : public ::testing::Test {
  protected:
	void SetUp() override
	{
		entity = registry.create();
		registry.register_component<PositionComponent>(entity, PositionComponent{{0.0f, 0.0f}});
		registry.register_component<AccelerationComponent>(entity, AccelerationComponent{{0.0f, 0.0f}});
		registry.register_component<MovementComponent>(entity, MovementComponent{});
	}

	void SetVelocity(float x, float y)
	{
		registry.register_component<VelocityComponent>(entity, VelocityComponent{{x, y}});
	}

	Registry registry;
	Entity entity;
};

TEST_F(PhysicsSystemTest, UpdatesPositionBasedOnFactors)
{
	SetVelocity(10.0f, 5.0f);

	PhysicsSystem::Update(registry, 1.0f);

	auto &pos = registry.get<PositionComponent>(entity);

	EXPECT_FLOAT_EQ(pos.position.x, 10.0f);
	EXPECT_FLOAT_EQ(pos.position.y, 5.0f);
}

TEST_F(PhysicsSystemTest, RespectsMaxSpeed)
{
	SetVelocity(1000.0f, 0.0f);

	PhysicsSystem::Update(registry, 1.0f);

	auto &pos = registry.get<PositionComponent>(entity);
	auto [acceleration, move] = registry.get<MovementComponent>(entity);
	const auto &[velocity] = registry.get<VelocityComponent>(entity);

	EXPECT_LE(std::abs(velocity.x), move);
	EXPECT_FLOAT_EQ(pos.position.x, velocity.x);
	EXPECT_FLOAT_EQ(pos.position.y, velocity.y);
}
