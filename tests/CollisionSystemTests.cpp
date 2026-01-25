#include "Engine/ECS/Systems/CollisionSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

#include <glm/glm.hpp>
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
		registry.registerComponent<PlayerTag>(dynamicEntity, {});
	}

	GameEngine::ECS::Entity createTileAtPosition(const glm::vec2 &position)
	{
		const auto tile = registry.create();
		registry.registerComponent(tile, PositionComponent{position});
		registry.registerComponent(tile, CollisionComponent{0, 0, collisionBoxSize, collisionBoxSize});
		return tile;
	}

	void resolveCollisions()
	{
		auto view = registry.view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent,
		                          GroundedComponent>();

		auto state = GameEngine::ECS::Systems::CollisionSystem::EntityPhysicsState{
		    dynamicEntity,
		    view.get<PositionComponent>(dynamicEntity),
		    view.get<VelocityComponent>(dynamicEntity),
		    view.get<AccelerationComponent>(dynamicEntity),
		    view.get<CollisionComponent>(dynamicEntity),
		    nullptr,
		    false};

		GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);
		GameEngine::ECS::Systems::CollisionSystem::resolveHorizontalCollisions(state);
		GameEngine::ECS::Systems::CollisionSystem::resolveVerticalCollisions(state);
		GameEngine::ECS::Systems::CollisionSystem::updateGroundedState(
		    state, view.get<GroundedComponent>(dynamicEntity), nullptr);
	}
};

TEST_F(CollisionSystemTest, PathObstructedWhenLineHitsTile)
{
	createTileAtPosition({50.0f, 0.0f});
	GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);

	const glm::vec2 start{0.0f, 50.0f};
	const glm::vec2 end{200.0f, 50.0f};
	const glm::vec2 projectileSize{4.0f, 4.0f};

	EXPECT_TRUE(GameEngine::ECS::Systems::CollisionSystem::isPathObstructed(start, end, projectileSize));
}

TEST_F(CollisionSystemTest, PathClearWhenNoTileInTheWay)
{
	createTileAtPosition({200.0f, 0.0f});
	GameEngine::ECS::Systems::CollisionSystem::buildStaticColliderGrid(registry);

	const glm::vec2 start{0.0f, 50.0f};
	const glm::vec2 end{150.0f, 50.0f};
	const glm::vec2 projectileSize{4.0f, 4.0f};

	EXPECT_FALSE(GameEngine::ECS::Systems::CollisionSystem::isPathObstructed(start, end, projectileSize));
}

TEST_F(CollisionSystemTest, NoCollisionWhenApart)
{
	const auto tileEntity = createTileAtPosition({collisionBoxSize, collisionBoxSize});
	auto tileView = registry.view<PositionComponent, CollisionComponent>();
	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {0.0f, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 200.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = tileView.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(collisionBoxSize, collisionBoxSize));
}

TEST_F(CollisionSystemTest, NoVerticalCollisionWhenApartVertical)
{
	const auto tileEntity = createTileAtPosition({0.0f, collisionBoxSize});
	auto tileView = registry.view<PositionComponent, CollisionComponent>();
	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {0.0f, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 200.0f));
	EXPECT_TRUE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = tileView.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(0.0f, collisionBoxSize));
}

TEST_F(CollisionSystemTest, NoHorizontalCollisionWhenApartHorizontal)
{
	const auto tileEntity = createTileAtPosition({collisionBoxSize, 0.0f});
	auto tileView = registry.view<PositionComponent, CollisionComponent>();
	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {0.0f, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 200.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = tileView.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(collisionBoxSize, 0.0f));
}

TEST_F(CollisionSystemTest, HorizontalCollisionFromLeftResolvesCorrectly)
{
	const auto tileEntity = createTileAtPosition({collisionBoxSize, 0.0f});
	auto tileView = registry.view<PositionComponent, CollisionComponent>();
	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {1.0f, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(0.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(0.0f, 200.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = tileView.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(collisionBoxSize, 0.0f));
}

TEST_F(CollisionSystemTest, HorizontalCollisionFromRightResolvesCorrectly)
{
	const auto tileEntity = createTileAtPosition({0.0f, 0.0f});
	auto tileView = registry.view<PositionComponent, CollisionComponent>();
	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {collisionBoxSize - 1, 0.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

	EXPECT_EQ(position, glm::vec2(collisionBoxSize, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(0.0f, 100.0f));
	EXPECT_EQ(velocity, glm::vec2(0.0f, 200.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = tileView.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(0.0f, 0.0f));
}

TEST_F(CollisionSystemTest, VerticalCollisionFromTopResolvesCorrectly)
{
	const auto tileEntity = createTileAtPosition({0.0f, collisionBoxSize});
	auto tileView = registry.view<PositionComponent, CollisionComponent>();
	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {0.0f, 1.0f};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

	EXPECT_EQ(position, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 0.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 0.0f));
	EXPECT_TRUE(grounded);

	auto &[tilePosition, tilePreviousPosition] = tileView.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(0, collisionBoxSize));
}

TEST_F(CollisionSystemTest, VerticalCollisionFromBottomResolvesCorrectly)
{
	const auto tileEntity = createTileAtPosition({0.0f, 0.0f});
	auto tileView = registry.view<PositionComponent, CollisionComponent>();
	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {0, collisionBoxSize - 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

	EXPECT_EQ(position, glm::vec2(0, collisionBoxSize));
	EXPECT_EQ(acceleration, glm::vec2(100.0f, 0.0f));
	EXPECT_EQ(velocity, glm::vec2(200.0f, 0.0f));
	EXPECT_FALSE(grounded);

	const auto &[tilePosition, tilePreviousPosition] = tileView.get<PositionComponent>(tileEntity);
	EXPECT_EQ(tilePosition, glm::vec2(0, 0));
}

TEST_F(CollisionSystemTest, TopLeftCornerCollisionResolvesCorrectly)
{
	createTileAtPosition({0.0f, 0.0f});
	createTileAtPosition({0.0f, collisionBoxSize});
	createTileAtPosition({collisionBoxSize, 0});

	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {collisionBoxSize - 1, collisionBoxSize - 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

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
	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {1, collisionBoxSize - 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

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

	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {collisionBoxSize - 1, 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

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

	auto view =
	    registry
	        .view<PositionComponent, VelocityComponent, AccelerationComponent, CollisionComponent, GroundedComponent>();
	auto &[position, previousPosition] = view.get<PositionComponent>(dynamicEntity);
	auto &[acceleration] = view.get<AccelerationComponent>(dynamicEntity);
	auto &[velocity] = view.get<VelocityComponent>(dynamicEntity);
	const auto &[grounded] = view.get<GroundedComponent>(dynamicEntity);

	position = {1, 1};
	acceleration = {100.0f, 100.0f};
	velocity = {200.0f, 200.0f};

	resolveCollisions();

	EXPECT_EQ(position, glm::vec2(0, 0));
	EXPECT_EQ(acceleration, glm::vec2(0.0f, 0.0f));
	EXPECT_EQ(velocity, glm::vec2(0.0f, 0.0f));
	EXPECT_TRUE(grounded);
}
