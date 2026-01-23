#include "Engine/ECS/Systems/Combat/HitDetectionSystem.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/Systems/Combat/HealthSystem.hpp"
#include "Engine/ECS/Systems/StateSystem.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

struct HitDetectionTest : testing::Test {
	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity attacker{};
	GameEngine::ECS::Entity enemy{};

	void SetUp() override
	{
		attacker = registry.create();
		enemy = registry.create();

		registry.registerComponent<PlayerTag>(attacker, {});
		registry.registerComponent<PositionComponent>(attacker, {{0.f, 0.f}});
		registry.registerComponent<HitBoxComponent>(
		    attacker, {.bodyHitBox = {0.f, 0.f, 20.f, 20.f}, .weaponHitBox = {0.f, 0.f, 20.f, 20.f}});
		registry.registerComponent<HurtBoxComponent>(attacker, {.hurtBox = {0.f, 0.f, 20.f, 20.f}});
		registry.registerComponent<AttackComponent>(attacker, {Timer{1s}, Timer{1s}});
		registry.registerComponent<StateComponent>(attacker, {});
		registry.registerComponent<HealthComponent>(attacker, {100, 100});
		registry.registerComponent<RenderComponent>(attacker, {.layer = 0, .flipX = false});
		AnimationComponent attackerAnim;
		attackerAnim.addAnimation(AnimationType::Hurting, {}, std::chrono::duration<float>(1.0f));
		registry.registerComponent(attacker, attackerAnim);

		registry.registerComponent<EnemyTag>(enemy, {});
		registry.registerComponent<PositionComponent>(enemy, {{500.f, 500.f}});
		registry.registerComponent<AttackComponent>(enemy, {Timer{1s}, Timer{1s}});
		registry.registerComponent<HealthComponent>(enemy, {100, 100});
		registry.registerComponent<HurtBoxComponent>(enemy, {.hurtBox = {0.f, 0.f, 20.f, 20.f}});
		registry.registerComponent<StateComponent>(enemy, {});

		AnimationComponent enemyAnim;
		enemyAnim.addAnimation(AnimationType::Hurting, {}, std::chrono::duration<float>(1.0f));
		registry.registerComponent(enemy, enemyAnim);
	}
};

TEST_F(HitDetectionTest, AttackerNotAttackingShouldNotDealDamage)
{
	auto &victimPos = registry.get<PositionComponent>(enemy);
	victimPos.position = {10.f, 0.f};

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};

	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);

	EXPECT_FALSE(callbackCalled);
	EXPECT_FALSE(registry.has<PendingDamageComponent>(enemy));
}

TEST_F(HitDetectionTest, AttackerIsAttackingShouldDealDamage)
{
	auto &attack = registry.get<AttackComponent>(attacker);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	auto &victimPos = registry.get<PositionComponent>(enemy);
	victimPos.position = {20.f, 0.f};

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};

	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	GameEngine::ECS::Systems::AttackSystem::update(registry, 0.1f, {});

	EXPECT_TRUE(callbackCalled);
	EXPECT_EQ(calledAttacker, attacker);
	EXPECT_EQ(calledVictim, enemy);
	EXPECT_TRUE(registry.has<PendingDamageComponent>(enemy));
}

TEST_F(HitDetectionTest, AttackerShouldNotDamageItself)
{
	auto &attack = registry.get<AttackComponent>(attacker);
	attack.attackTimer.reset();

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};
	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	EXPECT_FALSE(callbackCalled);
	EXPECT_FALSE(registry.has<PendingDamageComponent>(attacker));
}

TEST_F(HitDetectionTest, BodyHitBoxShouldAlwaysDealDamage)
{
	auto &passivePos = registry.get<PositionComponent>(enemy);
	registry.registerComponent<HitBoxComponent>(
	    enemy, {.bodyHitBox = {0.f, 0.f, 20.f, 20.f}, .weaponHitBox = {0.f, 0.f, 0.f, 0.f}});
	passivePos.position = {0.f, 0.f};

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};

	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);

	EXPECT_FALSE(callbackCalled);
	EXPECT_TRUE(registry.has<PendingDamageComponent>(attacker));
	EXPECT_FALSE(registry.has<PendingDamageComponent>(enemy));
}

TEST_F(HitDetectionTest, VictimIsDeadShouldNotApplyDamage)
{
	auto &passivePos = registry.get<PositionComponent>(enemy);
	registry.registerComponent<HitBoxComponent>(
	    enemy, {.bodyHitBox = {0.f, 0.f, 20.f, 20.f}, .weaponHitBox = {0.f, 0.f, 0.f, 0.f}});
	passivePos.position = {0.f, 0.f};
	auto &[victimState] = registry.get<StateComponent>(enemy);
	victimState = State::Dead;

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};

	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	EXPECT_FALSE(callbackCalled);
	EXPECT_FALSE(registry.has<PendingDamageComponent>(enemy));
}

TEST_F(HitDetectionTest, PlayerShouldNotDamagePlayer)
{
	auto player2 = registry.create();
	registry.registerComponent<PlayerTag>(player2, {});
	registry.registerComponent<PositionComponent>(player2, {{20.f, 0.f}}); // Overlapping with attacker
	registry.registerComponent<HurtBoxComponent>(player2, {.hurtBox = {0.f, 0.f, 20.f, 20.f}});
	registry.registerComponent<StateComponent>(player2, {});

	auto &attack = registry.get<AttackComponent>(attacker);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};

	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	EXPECT_FALSE(callbackCalled);
	EXPECT_FALSE(registry.has<PendingDamageComponent>(player2));
}

TEST_F(HitDetectionTest, EnemyShouldNotDamageEnemy)
{
	const auto enemy2 = registry.create();
	registry.registerComponent<EnemyTag>(enemy2, {});
	registry.registerComponent<PositionComponent>(enemy2, {{0.f, 0.f}});
	registry.registerComponent<HurtBoxComponent>(enemy2, {.hurtBox = {0.f, 0.f, 20.f, 20.f}});
	registry.registerComponent<StateComponent>(enemy2, {});

	registry.registerComponent<HitBoxComponent>(
	    enemy, {.bodyHitBox = {0.f, 0.f, 20.f, 20.f}, .weaponHitBox = {0.f, 0.f, 0.f, 0.f}});
	auto &enemyPos = registry.get<PositionComponent>(enemy);
	enemyPos.position = {0.f, 0.f};

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};

	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	EXPECT_FALSE(callbackCalled);
	EXPECT_FALSE(registry.has<PendingDamageComponent>(enemy2));
}

TEST_F(HitDetectionTest, NoOverlapShouldNotDealDamage)
{
	auto &attack = registry.get<AttackComponent>(attacker);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};
	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	EXPECT_FALSE(callbackCalled);
	EXPECT_FALSE(registry.has<PendingDamageComponent>(enemy));
}

TEST_F(HitDetectionTest, EdgeTouchingShouldDealDamage)
{
	auto &attack = registry.get<AttackComponent>(attacker);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	auto &enemyPos = registry.get<PositionComponent>(enemy);
	enemyPos.position = {20.f, 0.f};

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};
	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	EXPECT_TRUE(callbackCalled);
	EXPECT_EQ(calledAttacker, attacker);
	EXPECT_EQ(calledVictim, enemy);
	EXPECT_TRUE(registry.has<PendingDamageComponent>(enemy));
}

TEST_F(HitDetectionTest, FacingLeftShouldOffsetHitboxCorrectly)
{
	auto &render = registry.get<RenderComponent>(attacker);
	render.flipX = true;

	auto &attack = registry.get<AttackComponent>(attacker);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	auto &enemyPos = registry.get<PositionComponent>(enemy);
	enemyPos.position = {-10.f, 0.f};

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};
	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	EXPECT_TRUE(callbackCalled);
	EXPECT_EQ(calledAttacker, attacker);
	EXPECT_EQ(calledVictim, enemy);
	EXPECT_TRUE(registry.has<PendingDamageComponent>(enemy));
}

TEST_F(HitDetectionTest, BodyHitShouldSetVictimStateToHurting)
{
	registry.registerComponent<HitBoxComponent>(
	    enemy, {.bodyHitBox = {0.f, 0.f, 20.f, 20.f}, .weaponHitBox = {0.f, 0.f, 0.f, 0.f}});
	auto &enemyPos = registry.get<PositionComponent>(enemy);
	enemyPos.position = {0.f, 0.f};

	const auto &attackerState = registry.get<StateComponent>(attacker);

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};
	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	EXPECT_FALSE(callbackCalled);
	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1f);
	EXPECT_EQ(attackerState.state, State::Hurting);
}

TEST_F(HitDetectionTest, PendingDamageIsRemovedAfterProcessing)
{
	registry.registerComponent<HitBoxComponent>(
	    enemy, {.bodyHitBox = {0.f, 0.f, 20.f, 20.f}, .weaponHitBox = {0.f, 0.f, 0.f, 0.f}});
	auto &enemyPos = registry.get<PositionComponent>(enemy);
	enemyPos.position = {0.f, 0.f};

	registry.registerComponent<HealthComponent>(attacker, {100, 100, 1s});

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledAttacker{};
	GameEngine::ECS::Entity calledVictim{};
	auto mockCallback = [&](const GameEngine::ECS::Entity e1, const GameEngine::ECS::Entity e2) {
		callbackCalled = true;
		calledAttacker = e1;
		calledVictim = e2;
	};
	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);
	EXPECT_FALSE(callbackCalled);
	EXPECT_TRUE(registry.has<PendingDamageComponent>(attacker));

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1f);
	EXPECT_FALSE(registry.has<PendingDamageComponent>(attacker));
}

TEST_F(HitDetectionTest, ProjectileHitsPlayer)
{
	auto projectile = registry.create();
	registry.registerComponent<EnemyTag>(projectile, {});
	registry.registerComponent<ProjectileComponent>(projectile, {});
	registry.registerComponent<PositionComponent>(projectile, {{0.f, 0.f}});
	registry.registerComponent<HurtBoxComponent>(projectile, {{0.f, 0.f, 10.f, 10.f}});

	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, [](GameEngine::ECS::Entity, GameEngine::ECS::Entity) {});

	EXPECT_TRUE(registry.has<PendingDamageComponent>(attacker));
}

TEST_F(HitDetectionTest, ProjectileDoesNotHitWhenNoOverlap)
{
	auto projectile = registry.create();
	registry.registerComponent<EnemyTag>(projectile, {});
	registry.registerComponent<ProjectileComponent>(projectile, {});
	registry.registerComponent<PositionComponent>(projectile, {{500.f, 500.f}});
	registry.registerComponent<HurtBoxComponent>(projectile, {{0.f, 0.f, 10.f, 10.f}});

	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, [](GameEngine::ECS::Entity, GameEngine::ECS::Entity) {});

	EXPECT_FALSE(registry.has<PendingDamageComponent>(attacker));
}

TEST_F(HitDetectionTest, ProjectileDoesNotHitDeadVictim)
{
	auto projectile = registry.create();
	registry.registerComponent<EnemyTag>(projectile, {});
	registry.registerComponent<ProjectileComponent>(projectile, {});
	registry.registerComponent<PositionComponent>(projectile, {{0.f, 0.f}});
	registry.registerComponent<HurtBoxComponent>(projectile, {{0.f, 0.f, 10.f, 10.f}});

	auto &attackerState = registry.get<StateComponent>(attacker);
	attackerState.state = State::Dead;

	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, [](GameEngine::ECS::Entity, GameEngine::ECS::Entity) {});

	EXPECT_FALSE(registry.has<PendingDamageComponent>(attacker));
}
