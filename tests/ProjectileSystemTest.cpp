#include "Engine/ECS/Systems/Enemy/ProjectileSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Components/Tags/ProjectileTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Graphics/TextureManager.hpp"

#include <cmath>
#include <filesystem>
#include <gtest/gtest.h>
#include <numbers>

using namespace std::chrono_literals;

struct ProjectileSystemTest : testing::Test {
	GameEngine::ECS::Registry registry;
	TextureManager *textureManager = nullptr;
	GameEngine::ECS::Entity enemy{};
	GameEngine::ECS::Entity player{};
	AnimationFactory animationFactory{*textureManager};
	ProjectileFactory projectileFactory{registry, animationFactory};
	GameEngine::ECS::Systems::ProjectileSystem projectileSystem{projectileFactory};

	void SetUp() override
	{
		SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL);
		textureManager = dynamic_cast<TextureManager *>(this);
		enemy = registry.create();
		registry.registerComponent<EnemyTag>(enemy, {});
		registry.registerComponent<PositionComponent>(enemy, {{100.0f, 100.0f}});
		registry.registerComponent<HurtBoxComponent>(enemy, {{0.0f, 0.0f, 20.0f, 20.0f}});
		registry.registerComponent<AttackComponent>(enemy, {Timer{1s}, Timer{1s}});
		registry.registerComponent<RangedAttackComponent>(enemy, {});
		registry.registerComponent<ProjectileComponent>(enemy,
		                                                {.maxRange = 500.0f, .speed = 200.0f, .projectileTilesetPath = "dummy.tsj"});
		registry.registerComponent<RenderComponent>(enemy, {.flipX = false});
		registry.registerComponent<TransformComponent>(enemy, {.scale = {1.0f, 1.0f}});
		registry.registerComponent<SpriteComponent>(enemy, {});
		registry.registerComponent<HitBoxComponent>(enemy, {});

		player = registry.create();
		registry.registerComponent<PlayerTag>(player, {});
		registry.registerComponent<PositionComponent>(player, {{200.0f, 100.0f}});
		registry.registerComponent<CollisionComponent>(player, {{0.0f, 0.0f, 20.0f, 20.0f}});
		registry.registerComponent<TransformComponent>(player, {.scale = {1.0f, 1.0f}});
		registry.registerComponent<GroundedComponent>(player, {.grounded = true});
		registry.registerComponent<StateComponent>(player, {.state = State::Idle});
	}
};

TEST_F(ProjectileSystemTest, MovesProjectilesBasedOnVelocity)
{
	const auto projectile = registry.create();
	registry.registerComponent<ProjectileTag>(projectile, {});
	registry.registerComponent<ProjectileComponent>(projectile, {.maxRange = 1000.0f, .spawnPosition = {0.0f, 0.0f}});
	registry.registerComponent<VelocityComponent>(projectile, {{100.0f, 0.0f}});
	registry.registerComponent<PositionComponent>(projectile, {{0.0f, 0.0f}});

	projectileSystem.update(registry, 0.1f);

	auto view = registry.view<PositionComponent>();
	const auto &pos = view.get<PositionComponent>(projectile);
	EXPECT_FLOAT_EQ(pos.position.x, 10.0f);
	EXPECT_FLOAT_EQ(pos.position.y, 0.0f);
}

TEST_F(ProjectileSystemTest, DoesNotSpawnProjectileWhenNotAttacking)
{
	auto view = registry.view<AttackComponent>();
	auto &attack = view.get<AttackComponent>(enemy);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(2.0f), false);

	const size_t initialEntityCount = registry.size();
	projectileSystem.update(registry, 0.1f);

	EXPECT_EQ(registry.size(), initialEntityCount);
}

TEST_F(ProjectileSystemTest, SpawnsProjectileWhenAttacking)
{
	auto view = registry.view<AttackComponent>();
	auto &attack = view.get<AttackComponent>(enemy);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	size_t initialEntityCount = registry.size();
	projectileSystem.update(registry, 0.1f);

	EXPECT_GT(registry.size(), initialEntityCount);
}

TEST_F(ProjectileSystemTest, SpawnsProjectileTowardTargetSnapshot)
{
	auto view = registry.view<AttackComponent, RangedAttackComponent>();
	auto &attack = view.get<AttackComponent>(enemy);
	auto &rangedAttack = view.get<RangedAttackComponent>(enemy);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);
	rangedAttack.targetSnapshot = {400.0f, 100.0f};

	projectileSystem.update(registry, 0.1f);

	auto projectileView = registry.view<ProjectileTag, ProjectileComponent, VelocityComponent>();
	ASSERT_GT(projectileView.size(), 0u);
	const auto projectile = projectileView.at(0);
	const auto &velocity = projectileView.get<VelocityComponent>(projectile);
	EXPECT_GT(velocity.velocity.x, 0.0f);
}

TEST_F(ProjectileSystemTest, SpawnsProjectileTowardUpperRightTarget)
{
	auto view =
	    registry.view<AttackComponent, RangedAttackComponent, HurtBoxComponent, PositionComponent, RenderComponent>();
	auto &attack = view.get<AttackComponent>(enemy);
	auto &rangedAttack = view.get<RangedAttackComponent>(enemy);
	auto &hurtBox = view.get<HurtBoxComponent>(enemy);
	auto &position = view.get<PositionComponent>(enemy);
	auto &render = view.get<RenderComponent>(enemy);

	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);
	hurtBox.hurtBox = {0.0f, 0.0f, 20.0f, 20.0f};
	position.position = {0.0f, 0.0f};
	render.flipX = false;
	rangedAttack.projectileOffset = {0.0f, 0.0f};
	rangedAttack.targetSnapshot = {100.0f, -100.0f};

	projectileSystem.update(registry, 0.1f);

	auto projectileView = registry.view<ProjectileTag, VelocityComponent>();
	ASSERT_GT(projectileView.size(), 0u);
	const auto projectile = projectileView.at(0);
	const auto &velocity = projectileView.get<VelocityComponent>(projectile);
	const glm::vec2 dir = glm::normalize(velocity.velocity);
	EXPECT_GT(dir.x, 0.0f);
	EXPECT_LT(dir.y, 0.0f);
}

TEST_F(ProjectileSystemTest, SpawnsProjectileTowardLowerRightTarget)
{
	auto view =
	    registry.view<AttackComponent, RangedAttackComponent, HurtBoxComponent, PositionComponent, RenderComponent>();
	auto &attack = view.get<AttackComponent>(enemy);
	auto &rangedAttack = view.get<RangedAttackComponent>(enemy);
	auto &hurtBox = view.get<HurtBoxComponent>(enemy);
	auto &position = view.get<PositionComponent>(enemy);
	auto &render = view.get<RenderComponent>(enemy);

	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);
	hurtBox.hurtBox = {0.0f, 0.0f, 20.0f, 20.0f};
	position.position = {0.0f, 0.0f};
	render.flipX = false;
	rangedAttack.projectileOffset = {0.0f, 0.0f};
	rangedAttack.targetSnapshot = {100.0f, 100.0f};

	projectileSystem.update(registry, 0.1f);

	auto projectileView = registry.view<ProjectileTag, VelocityComponent>();
	ASSERT_GT(projectileView.size(), 0u);
	const auto projectile = projectileView.at(0);
	const auto &velocity = projectileView.get<VelocityComponent>(projectile);
	const glm::vec2 dir = glm::normalize(velocity.velocity);
	EXPECT_GT(dir.x, 0.0f);
	EXPECT_GT(dir.y, 0.0f);
}

TEST_F(ProjectileSystemTest, SpawnsProjectileTowardUpperLeftTargetWhenFacingLeft)
{
	auto view =
	    registry.view<AttackComponent, RangedAttackComponent, HurtBoxComponent, PositionComponent, RenderComponent>();
	auto &attack = view.get<AttackComponent>(enemy);
	auto &rangedAttack = view.get<RangedAttackComponent>(enemy);
	auto &hurtBox = view.get<HurtBoxComponent>(enemy);
	auto &position = view.get<PositionComponent>(enemy);
	auto &render = view.get<RenderComponent>(enemy);

	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);
	hurtBox.hurtBox = {0.0f, 0.0f, 20.0f, 20.0f};
	position.position = {0.0f, 0.0f};
	render.flipX = true;
	rangedAttack.projectileOffset = {0.0f, 0.0f};
	rangedAttack.targetSnapshot = {-100.0f, -100.0f};

	projectileSystem.update(registry, 0.1f);

	auto projectileView = registry.view<ProjectileTag, VelocityComponent>();
	ASSERT_GT(projectileView.size(), 0u);
	const auto projectile = projectileView.at(0);
	const auto &velocity = projectileView.get<VelocityComponent>(projectile);
	const glm::vec2 dir = glm::normalize(velocity.velocity);
	EXPECT_LT(dir.x, 0.0f);
	EXPECT_LT(dir.y, 0.0f);
}

TEST_F(ProjectileSystemTest, SetsProjectileRotationFromTargetSnapshot)
{
	auto view = registry.view<AttackComponent, RangedAttackComponent, PositionComponent, HurtBoxComponent,
	                          ProjectileComponent, SpriteComponent, HitBoxComponent>();
	auto &attack = view.get<AttackComponent>(enemy);
	auto &rangedAttack = view.get<RangedAttackComponent>(enemy);
	auto &position = view.get<PositionComponent>(enemy);
	auto &hurtBox = view.get<HurtBoxComponent>(enemy);
	auto &sprite = view.get<SpriteComponent>(enemy);
	auto &hitBox = view.get<HitBoxComponent>(enemy);
	auto &projectileSettings = view.get<ProjectileComponent>(enemy);

	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	position.position = {0.0f, 0.0f};
	hurtBox.hurtBox = {0.0f, 0.0f, 20.0f, 20.0f};
	sprite.tileSize = {10.0f, 10.0f};
	hitBox = {0.0f, 0.0f};
	rangedAttack.projectileOffset = {0.0f, 0.0f};
	registry.get<RangedAttackComponent>(enemy).targetSnapshot = {0.0f, 100.0f};

	projectileSystem.update(registry, 0.1f);

	auto projectileView = registry.view<ProjectileTag, TransformComponent>();
	ASSERT_GT(projectileView.size(), 0u);
	const auto projectile = projectileView.at(0);
	const auto &transform = projectileView.get<TransformComponent>(projectile);

	// Re-fetch component to avoid any dangling reference issues
	const auto &rangedAttackPost = registry.get<RangedAttackComponent>(enemy);
	const auto &hurtBoxPost = registry.get<HurtBoxComponent>(enemy);
	const auto &posPost = registry.get<PositionComponent>(enemy);
	
	glm::vec2 spawnPos = posPost.position;
	spawnPos.x += (hurtBoxPost.hurtBox.x + hurtBoxPost.hurtBox.w);
	spawnPos.y += hurtBoxPost.hurtBox.y;

	// Projectile internal offset (dummy.tsj uses default empty data: 3x3 hitbox)
	const auto &data = ProjectileFactory::getCachedProjectileData("dummy.tsj");
	const glm::vec2 pScale = data.scale;
	const glm::vec2 hitboxOffset = {data.hitbox.x * pScale.x, data.hitbox.y * pScale.y};
	const glm::vec2 projectileSize = {data.hitbox.w * pScale.x, data.hitbox.h * pScale.y};
	
	const glm::vec2 spawnCenter = spawnPos + hitboxOffset + (projectileSize * 0.5f); 

	const glm::vec2 direction = glm::normalize(rangedAttackPost.targetSnapshot - spawnCenter);
	const double expectedRotation = std::atan2(direction.y, direction.x) * (180.0 / std::numbers::pi);

	double diff = std::fmod(std::abs(transform.rotation - expectedRotation), 360.0);
	if (diff > 180.0) diff = 360.0 - diff;
	EXPECT_NEAR(diff, 0.0, 0.1);
}

TEST_F(ProjectileSystemTest, MarksProjectileAsSpawned)
{
	auto view = registry.view<AttackComponent, RangedAttackComponent>();
	auto &attack = view.get<AttackComponent>(enemy);
	auto &rangedAttack = view.get<RangedAttackComponent>(enemy);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	EXPECT_FALSE(rangedAttack.projectileSpawned);
	projectileSystem.update(registry, 0.1f);
	EXPECT_TRUE(rangedAttack.projectileSpawned);
}

TEST_F(ProjectileSystemTest, ResetsSpawnedFlagWhenNotAttacking)
{
	auto view = registry.view<AttackComponent, RangedAttackComponent>();

	auto &attack = view.get<AttackComponent>(enemy);
	auto &rangedAttack = view.get<RangedAttackComponent>(enemy);
	rangedAttack.projectileSpawned = true;

	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(2.0f), false);

	projectileSystem.update(registry, 0.1f);
	EXPECT_FALSE(rangedAttack.projectileSpawned);
}

TEST_F(ProjectileSystemTest, DeletesProjectileWhenExceedingMaxRange)
{
	const auto projectile = registry.create();
	registry.registerComponent<ProjectileTag>(projectile, {});
	registry.registerComponent<EnemyTag>(projectile, {});
	registry.registerComponent<ProjectileComponent>(projectile, {.maxRange = 100.0f, .spawnPosition = {0.0f, 0.0f}});
	registry.registerComponent<PositionComponent>(projectile, {{150.0f, 0.0f}});

	projectileSystem.update(registry, 0.1f);

	EXPECT_FALSE(registry.has<ProjectileComponent>(projectile));
}

TEST_F(ProjectileSystemTest, KeepsProjectileWhenWithinRange)
{
	auto projectile = registry.create();
	registry.registerComponent<ProjectileTag>(projectile, {});
	registry.registerComponent<EnemyTag>(projectile, {});
	registry.registerComponent<ProjectileComponent>(projectile, {.maxRange = 100.0f, .spawnPosition = {0.0f, 0.0f}});
	registry.registerComponent<PositionComponent>(projectile, {{50.0f, 0.0f}});

	projectileSystem.update(registry, 0.1f);

	EXPECT_TRUE(registry.has<ProjectileComponent>(projectile));
}

TEST_F(ProjectileSystemTest, IgnoresRangedEnemySettingsInProjectileMovement)
{
	registry.registerComponent<VelocityComponent>(enemy, {{50.0f, 0.0f}});
	auto view = registry.view<PositionComponent>();
	auto &pos = view.get<PositionComponent>(enemy);
	const glm::vec2 start = pos.position;

	projectileSystem.update(registry, 0.1f);

	EXPECT_EQ(pos.position.x, start.x);
	EXPECT_EQ(pos.position.y, start.y);
}
