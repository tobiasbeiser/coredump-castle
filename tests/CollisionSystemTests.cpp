#include "Engine/ECS/Systems/CollisionSystem.hpp"

#include <gtest/gtest.h>
class CollisionSystemTest : public ::testing::Test {
  public:
	const float collisionBoxSize = 100.0f;
	const float acceleration = 100.0f;
	GameEngine::ECS::Entity dynamicEntity = 0;
	GameEngine::ECS::Registry registry;

  protected:
	void SetUp() override
	{
		dynamicEntity = registry.create();
		registry.registerComponent(dynamicEntity, PositionComponent{});
		registry.registerComponent(dynamicEntity, VelocityComponent{});
		registry.registerComponent(dynamicEntity, AccelerationComponent{});
		registry.registerComponent(dynamicEntity, CollisionComponent{0, 0, collisionBoxSize, collisionBoxSize});
		registry.registerComponent(dynamicEntity, GroundedComponent{});
	}

	GameEngine::ECS::Entity createTileAtPosition(const glm::vec2 &position)
	{
		const auto tile = registry.create();
		registry.registerComponent(tile, PositionComponent{position});
		registry.registerComponent(tile, CollisionComponent{0, 0, collisionBoxSize, collisionBoxSize});
		return tile;
	}
};

TEST_F(CollisionSystemTest, NoCollisionWhenApart)
{
	const auto tileEntity = createTileAtPosition({collisionBoxSize, collisionBoxSize});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {0.0f, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 200.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = registry.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(collisionBoxSize, collisionBoxSize));
}

TEST_F(CollisionSystemTest, NoVerticalCollisionWhenApartVertical)
{
	const auto tileEntity = createTileAtPosition({0.0f, collisionBoxSize});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {0.0f, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 200.0f));
	EXPECT_TRUE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = registry.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(0.0f, collisionBoxSize));
}

TEST_F(CollisionSystemTest, NoHorizontalCollisionWhenApartHorizontal)
{
	const auto tileEntity = createTileAtPosition({collisionBoxSize, 0.0f});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {0.0f, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 200.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = registry.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(collisionBoxSize, 0.0f));
}

TEST_F(CollisionSystemTest, HorizontalCollisionFromLeftResolvesCorrectly)
{
	const auto tileEntity = createTileAtPosition({collisionBoxSize, 0.0f});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {1.0f, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(0.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(0.0f, 200.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = registry.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(collisionBoxSize, 0.0f));
}

TEST_F(CollisionSystemTest, HorizontalCollisionFromRightResolvesCorrectly)
{
	const auto tileEntity = createTileAtPosition({0.0f, 0.0f});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {collisionBoxSize - 1, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(collisionBoxSize, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(0.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(0.0f, 200.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = registry.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(0.0f, 0.0f));
}

TEST_F(CollisionSystemTest, VerticalCollisionFromTopResolvesCorrectly)
{
	const auto tileEntity = createTileAtPosition({0.0f, collisionBoxSize});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {0.0f, 1.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 0.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 0.0f));
	EXPECT_TRUE(grounded);

	auto &[tilePosition, tilePreviousPosition] = registry.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(0, collisionBoxSize));
}

TEST_F(CollisionSystemTest, VerticalCollisionFromBottomResolvesCorrectly)
{
	const auto tileEntity = createTileAtPosition({0.0f, 0.0f});
	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {0, collisionBoxSize - 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(0, collisionBoxSize));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 0.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 0.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = registry.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(0, 0));
}

TEST_F(CollisionSystemTest, TopLeftCornerCollisionResolvesCorrectly)
{
	createTileAtPosition({0.0f, 0.0f});
	createTileAtPosition({0.0f, collisionBoxSize});
	createTileAtPosition({collisionBoxSize, 0});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {collisionBoxSize - 1, collisionBoxSize - 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(collisionBoxSize, collisionBoxSize));
	EXPECT_EQ(acceleration, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(velocity, glm::vec2(0.0f, 0.0f));
	EXPECT_FALSE(grounded);
}

TEST_F(CollisionSystemTest, TopRightCornerCollisionResolvesCorrectly)
{
	createTileAtPosition({0.0f, 0.0f});
	createTileAtPosition({collisionBoxSize, 0.0f});
	createTileAtPosition({collisionBoxSize, collisionBoxSize});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {1, collisionBoxSize - 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(0, collisionBoxSize));
	EXPECT_EQ(acceleration, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(velocity, glm::vec2(0.0f, 0.0f));
	EXPECT_FALSE(grounded);
}

TEST_F(CollisionSystemTest, BottomLeftCornerCollisionResolvesCorrectly)
{
	createTileAtPosition({0.0f, 0.0f});
	createTileAtPosition({0.0f, collisionBoxSize});
	createTileAtPosition({collisionBoxSize, collisionBoxSize});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {collisionBoxSize - 1, 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(collisionBoxSize, 0));
	EXPECT_EQ(acceleration, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(velocity, glm::vec2(0.0f, 0.0f));
	EXPECT_TRUE(grounded);
}

TEST_F(CollisionSystemTest, BottomRightCornerCollisionResolvesCorrectly)
{
	createTileAtPosition({0.0f, collisionBoxSize});
	createTileAtPosition({collisionBoxSize, 0});
	createTileAtPosition({collisionBoxSize, collisionBoxSize});

	auto &[position, previousPosition] = registry.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = registry.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = registry.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = registry.get<GroundedComponent>(dynamicEntity);

	position = {1, 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	GameEngine::ECS::Systems::CollisionSystem::update(registry);

	EXPECT_EQ(position, glm::vec2(0, 0));
	EXPECT_EQ(acceleration, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(velocity, glm::vec2(0.0f, 0.0f));
	EXPECT_TRUE(grounded);
}
