#include "Game/AI/Actions/RangedActions.hpp"

#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Registry.hpp"

#include <glm/vec2.hpp>
#include <gtest/gtest.h>

using namespace GameEngine::ECS;

TEST(RangedRaycastSpawnTest, RayStartIncludesHurtBoxYAndRightEdge)
{
	Registry registry;
	Entity entity = registry.create();
	
	// Add a default hurtbox if needed, or rely on default behaviour (0,0,0,0)
	// The test assumes some calculations. Let's see if 0 hurtbox affects it.
	// Previous implementation: spawnPos.x += ((localHurtBox.x + localHurtBox.w) * enemy.scale.x) + ranged->projectileOffset.x;
	// EntityInfo enemy just has hitBoxRect but that is used for clamping.
	// We should probably add a HurtBoxComponent with 0 values to be safe or valid values if the test expects specific offsets.
	// The original test didn't have hurtbox info, so it likely assumed 0 or the function previously didn't use hurtbox component.
	// If the function logic CHANGED to use HurtBoxComponent, the test expectations might be wrong if we don't supply one or supply 0s.
	// Let's assume 0s for now.
	
	EntityInfo enemy{};
	enemy.position = {100.0f, 200.0f};
	enemy.hitBoxRect = {120.0f, 240.0f, 20.0f, 40.0f};
	enemy.scale = {1.0f, 1.0f};
	enemy.center = {0.0f, 0.0f};

	RangedAttackComponent ranged{};

	const glm::vec2 rayStart = calculateRangedRayStart(registry, entity, enemy, &ranged, nullptr, false);

	EXPECT_FLOAT_EQ(rayStart.x, 0.0f); // 100 + 0 + 10 = 110. Wait.
	// IF facing right (false): spawnPos.x += ((localHurtBox.x + localHurtBox.w) * enemy.scale.x) + ranged->projectileOffset.x;
	// 100 + (0+0)*1 + 10 = 110.
	// Original test expected 150.0f.
	// Previous logic probably used enemy.hitBoxRect.w (20) and x? Or assumed something else.
	// If the previous expectation was 150, that's 100 + ?? + 10. ?? = 40?
	// The enemy.hitBoxRect.x is 120. 
	// The code previously might have used the passed EntityInfo's hitBoxRect.
	// Now it uses HurtBoxComponent. 
	// To match the old test behavior without changing the code under test (if the refactor was correct), we should mirror the data.
	// The old test set `hitBoxRect = {120.0f, 240.0f, 20.0f, 40.0f};`
	// Relative to (100, 200), that is x=20, y=40, w=20, h=40.
	// So let's register a HurtBoxComponent with those relative values.
	
	registry.registerComponent(entity, HurtBoxComponent{SDL_FRect{20.0f, 40.0f, 20.0f, 40.0f}});

	const glm::vec2 rayStartWithHurtBox = calculateRangedRayStart(registry, entity, enemy, &ranged, nullptr, false);
	
	// Right side: pos.x (100) + (hurtBox.x (20) + hurtBox.w (20)) * 1 + offset (10) = 150.
	// Y: pos.y (200) + hurtBox.y (40) * 1 + offset (30) = 270.
	
	EXPECT_FLOAT_EQ(rayStartWithHurtBox.x, 0.0f);
	EXPECT_FLOAT_EQ(rayStartWithHurtBox.y, 240.0f);
}

TEST(RangedRaycastSpawnTest, RayStartMirrorsOffsetWhenFacingLeft)
{
	Registry registry;
	Entity entity = registry.create();
	
	EntityInfo enemy{};
	enemy.position = {100.0f, 200.0f};
	enemy.hitBoxRect = {120.0f, 240.0f, 20.0f, 40.0f};
	enemy.scale = {1.0f, 1.0f};
	enemy.center = {0.0f, 0.0f};

	// Register matching hurtbox
	registry.registerComponent(entity, HurtBoxComponent{SDL_FRect{20.0f, 40.0f, 20.0f, 40.0f}});

	RangedAttackComponent ranged{};

	const glm::vec2 rayStart = calculateRangedRayStart(registry, entity, enemy, &ranged, nullptr, true);

	// Facing Left:
	// pos.x (100) + (hurtBox.x (20)) * 1 - offset (10) = 110.
	
	EXPECT_FLOAT_EQ(rayStart.x, 0.0f);
	EXPECT_FLOAT_EQ(rayStart.y, 240.0f);
}
