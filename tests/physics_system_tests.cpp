#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/PhysicsSystem.hpp"
#include "Engine/ECS/View.hpp"
#include <gtest/gtest.h>

using namespace GameEngine::ECS;
using namespace GameEngine::ECS::Systems;

struct PhysicsSystemTest : public ::testing::Test {

	void SetUp() override
	{
		entity = registry.create();
		registry.registerComponent(entity, PositionComponent{{0.0f, 0.0f}});
		registry.registerComponent(entity, AccelerationComponent{{0.0f, 0.0f}});
		registry.registerComponent(entity, MovementComponent{});
	}

	void setVelocity(float x, float y) { registry.registerComponent(entity, VelocityComponent{{x, y}}); }

	Registry registry;
	Entity entity{};
};

TEST_F(PhysicsSystemTest, UpdatesPositionBasedOnFactors)
{
	setVelocity(10.0f, 5.0f);

	PhysicsSystem::update(registry, 1.0f);
	auto view = registry.view<PositionComponent>();
	auto &pos = view.get<PositionComponent>(entity);

	EXPECT_FLOAT_EQ(pos.position.x, 10.0f);
	EXPECT_FLOAT_EQ(pos.position.y, 5.0f);
}

TEST_F(PhysicsSystemTest, RespectsMaxSpeed)
{
	setVelocity(1000.0f, 0.0f);

	PhysicsSystem::update(registry, 1.0f);

	auto view = registry.view<PositionComponent, VelocityComponent, MovementComponent>();

	auto &pos = view.get<PositionComponent>(entity);
	auto [acceleration, move] = view.get<MovementComponent>(entity);
	const auto &[velocity] = view.get<VelocityComponent>(entity);

	EXPECT_LE(std::abs(velocity.x), move);
	EXPECT_FLOAT_EQ(pos.position.x, velocity.x);
	EXPECT_FLOAT_EQ(pos.position.y, velocity.y);
}
