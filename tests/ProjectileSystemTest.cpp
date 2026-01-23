#include "Engine/ECS/Systems/Enemy/ProjectileSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Graphics/TextureManager.hpp"

#include <gtest/gtest.h>

using namespace std::chrono_literals;

class ProjectileSystemTest : public ::testing::Test {
  protected:
	GameEngine::ECS::Registry registry;
	TextureManager *textureManager = nullptr;
	GameEngine::ECS::Entity enemy{};
	GameEngine::ECS::Entity player{};

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
		registry.registerComponent<ProjectileComponent>(enemy, {.maxRange = 500.0f, .speed = 200.0f});
		registry.registerComponent<RenderComponent>(enemy, {.flipX = false});
		registry.registerComponent<TransformComponent>(enemy, {.scale = {1.0f, 1.0f}});

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
	registry.registerComponent<ProjectileComponent>(projectile, {});
	registry.registerComponent<VelocityComponent>(projectile, {{100.0f, 0.0f}});
	registry.registerComponent<PositionComponent>(projectile, {{0.0f, 0.0f}});

	GameEngine::ECS::Systems::ProjectileSystem::update(registry, 0.1f, *textureManager);

	const auto &pos = registry.get<PositionComponent>(projectile);
	EXPECT_FLOAT_EQ(pos.position.x, 10.0f);
	EXPECT_FLOAT_EQ(pos.position.y, 0.0f);
}

TEST_F(ProjectileSystemTest, DoesNotSpawnProjectileWhenNotAttacking)
{
	auto &attack = registry.get<AttackComponent>(enemy);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(2.0f), false);

	const size_t initialEntityCount = registry.size();
	GameEngine::ECS::Systems::ProjectileSystem::update(registry, 0.1f, *textureManager);

	EXPECT_EQ(registry.size(), initialEntityCount);
}

TEST_F(ProjectileSystemTest, SpawnsProjectileWhenAttacking)
{
	auto &attack = registry.get<AttackComponent>(enemy);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	size_t initialEntityCount = registry.size();
	GameEngine::ECS::Systems::ProjectileSystem::update(registry, 0.1f, *textureManager);

	EXPECT_GT(registry.size(), initialEntityCount);
}

TEST_F(ProjectileSystemTest, SpawnsProjectileTowardTargetSnapshot)
{
	auto &attack = registry.get<AttackComponent>(enemy);
	auto &rangedAttack = registry.get<RangedAttackComponent>(enemy);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);
	rangedAttack.targetSnapshot = {400.0f, 100.0f};

	GameEngine::ECS::Systems::ProjectileSystem::update(registry, 0.1f, *textureManager);

	auto projectileView = registry.view<ProjectileComponent, VelocityComponent>().exclude<RangedAttackComponent>();
	ASSERT_GT(projectileView.size(), 0u);
	const auto projectile = projectileView.at(0);
	const auto &velocity = registry.get<VelocityComponent>(projectile);
	EXPECT_GT(velocity.velocity.x, 0.0f);
}

TEST_F(ProjectileSystemTest, MarksProjectileAsSpawned)
{
	auto &attack = registry.get<AttackComponent>(enemy);
	auto &rangedAttack = registry.get<RangedAttackComponent>(enemy);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	EXPECT_FALSE(rangedAttack.projectileSpawned);
	GameEngine::ECS::Systems::ProjectileSystem::update(registry, 0.1f, *textureManager);
	EXPECT_TRUE(rangedAttack.projectileSpawned);
}

TEST_F(ProjectileSystemTest, ResetsSpawnedFlagWhenNotAttacking)
{
	auto &attack = registry.get<AttackComponent>(enemy);
	auto &rangedAttack = registry.get<RangedAttackComponent>(enemy);
	rangedAttack.projectileSpawned = true;

	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(2.0f), false);

	GameEngine::ECS::Systems::ProjectileSystem::update(registry, 0.1f, *textureManager);
	EXPECT_FALSE(rangedAttack.projectileSpawned);
}

TEST_F(ProjectileSystemTest, DeletesProjectileWhenExceedingMaxRange)
{
	const auto projectile = registry.create();
	registry.registerComponent<EnemyTag>(projectile, {});
	registry.registerComponent<ProjectileComponent>(projectile, {.maxRange = 100.0f, .spawnPosition = {0.0f, 0.0f}});
	registry.registerComponent<PositionComponent>(projectile, {{150.0f, 0.0f}});

	GameEngine::ECS::Systems::ProjectileSystem::update(registry, 0.1f, *textureManager);

	EXPECT_FALSE(registry.has<ProjectileComponent>(projectile));
}

TEST_F(ProjectileSystemTest, KeepsProjectileWhenWithinRange)
{
	auto projectile = registry.create();
	registry.registerComponent<EnemyTag>(projectile, {});
	registry.registerComponent<ProjectileComponent>(projectile, {.maxRange = 100.0f, .spawnPosition = {0.0f, 0.0f}});
	registry.registerComponent<PositionComponent>(projectile, {{50.0f, 0.0f}});

	GameEngine::ECS::Systems::ProjectileSystem::update(registry, 0.1f, *textureManager);

	EXPECT_TRUE(registry.has<ProjectileComponent>(projectile));
}
