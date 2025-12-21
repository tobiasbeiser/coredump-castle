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
	hash.query({0, 0, 32, 32}, [&](Entity, const SDL_FRect &) { ++count; });
	EXPECT_EQ(count, 0);
}

TEST_F(SpatialHashTest, InsertAndQuerySingleEntity)
{
	constexpr Entity e{1};
	hash.insert(e, {10, 10, 20, 20});

	Entity found{0};
	hash.query({0, 0, 50, 50}, [&](const Entity entity, const SDL_FRect &) { found = entity; });

	EXPECT_EQ(found, 1);
}

TEST_F(SpatialHashTest, QueryExcludingSelf)
{
	constexpr Entity e1{1};
	constexpr Entity e2{2};
	hash.insert(e1, {10, 10, 20, 20});
	hash.insert(e2, {15, 15, 20, 20});

	std::vector<uint32_t> results;
	hash.queryExcluding({0, 0, 50, 50}, e1, [&](const Entity entity, const SDL_FRect &) { results.push_back(entity); });

	EXPECT_EQ(results.size(), 1);
	EXPECT_EQ(results[0], 2);
}

TEST_F(SpatialHashTest, EntitySpanningMultipleCells)
{
	constexpr Entity e{1};
	hash.insert(e, {0, 0, 200, 200});

	int count = 0;
	hash.query({150, 150, 10, 10}, [&](Entity, const SDL_FRect &) { ++count; });

	EXPECT_GT(count, 0);
}

TEST_F(SpatialHashTest, ClearRemovesAllEntries)
{
	hash.insert(Entity{1}, {10, 10, 20, 20});
	hash.clear();

	int count = 0;
	hash.query({0, 0, 100, 100}, [&](Entity, const SDL_FRect &) { ++count; });

	EXPECT_EQ(count, 0);
}
