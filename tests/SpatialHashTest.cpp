#include "Engine/Utils/SpatialHash.hpp"
#include <gtest/gtest.h>

using namespace GameEngine::Utils;
using namespace GameEngine::ECS;

class SpatialHashTest : public ::testing::Test {
  protected:
	SpatialHash hash{64.0f};
};

TEST_F(SpatialHashTest, QueryEmptyGridReturnsNothing)
{
	int count = 0;
	hash.query({0, 0, 32, 32}, [&](const Entity, const SDL_FRect &, ColliderType type) { ++count; });
	EXPECT_EQ(count, 0);
}

TEST_F(SpatialHashTest, InsertAndQuerySingleEntity)
{
	constexpr Entity e{1};
	hash.insert(e, {10, 10, 20, 20});

	SDL_FRect found{0, 0, 0, 0};
	hash.query({0, 0, 50, 50}, [&](const Entity, const SDL_FRect &box, ColliderType type) { found = box; });

	EXPECT_FLOAT_EQ(found.x, 10.0f);
}

TEST_F(SpatialHashTest, QueryExcludingSelf)
{
	constexpr Entity e1{1};
	constexpr Entity e2{2};
	hash.insert(e1, {10, 10, 20, 20});
	hash.insert(e2, {15, 15, 20, 20});

	std::vector<SDL_FRect> results;
	hash.queryExcluding({0, 0, 50, 50}, e1,
	                    [&](const Entity, const SDL_FRect &box, ColliderType type) { results.push_back(box); });

	EXPECT_EQ(results.size(), 1);
	EXPECT_FLOAT_EQ(results[0].x, 15.0f);
}

TEST_F(SpatialHashTest, EntitySpanningMultipleCells)
{
	constexpr Entity e{1};
	hash.insert(e, {0, 0, 200, 200});

	int count = 0;
	hash.query({150, 150, 10, 10}, [&](const Entity, const SDL_FRect &, ColliderType type) { ++count; });

	EXPECT_GT(count, 0);
}

TEST_F(SpatialHashTest, ClearRemovesAllEntries)
{
	hash.insert(Entity{1}, {10, 10, 20, 20});
	hash.clear();

	int count = 0;
	hash.query({0, 0, 100, 100}, [&](const Entity, const SDL_FRect &, ColliderType type) { ++count; });

	EXPECT_EQ(count, 0);
}
