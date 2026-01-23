#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/HealthSystem.hpp"
#include "Engine/ECS/Systems/Combat/HitDetectionSystem.hpp"
#include "Engine/ECS/View.hpp"

#include <gtest/gtest.h>

using namespace std::chrono_literals;

struct HitDetectionMultiTargetTest : testing::Test {

	GameEngine::ECS::Registry registry;
	GameEngine::ECS::Entity attacker{};
	GameEngine::ECS::Entity enemy1{};
	GameEngine::ECS::Entity enemy2{};

	void SetUp() override
	{
		attacker = registry.create();
		enemy1 = registry.create();
		enemy2 = registry.create();

		registry.registerComponent<PlayerTag>(attacker, {});
		registry.registerComponent<PositionComponent>(attacker, {{0.f, 0.f}});
		registry.registerComponent<HitBoxComponent>(
		    attacker, {.bodyHitBox = {0.f, 0.f, 20.f, 20.f}, .weaponHitBox = {0.f, 0.f, 100.f, 100.f}});
		// Wide attack to hit both
		registry.registerComponent<HurtBoxComponent>(attacker, {.hurtBox = {0.f, 0.f, 20.f, 20.f}});
		registry.registerComponent<AttackComponent>(attacker, {Timer{1s}, Timer{1s}});
		registry.registerComponent<HealthComponent>(attacker, {100, 100});
		registry.registerComponent<RenderComponent>(attacker, {});

		// Enemy 1 Setup
		registry.registerComponent<EnemyTag>(enemy1, {});
		registry.registerComponent<PositionComponent>(enemy1, {{10.f, 0.f}});
		registry.registerComponent<HealthComponent>(enemy1, {100, 100});
		registry.registerComponent<HitBoxComponent>(enemy1, {});
		registry.registerComponent<HurtBoxComponent>(enemy1, {.hurtBox = {0.f, 0.f, 20.f, 20.f}});

		// Enemy 2 Setup
		registry.registerComponent<EnemyTag>(enemy2, {});
		registry.registerComponent<PositionComponent>(enemy2, {{20.f, 0.f}});
		registry.registerComponent<HealthComponent>(enemy2, {100, 100});
		registry.registerComponent<HitBoxComponent>(enemy2, {});
		registry.registerComponent<HurtBoxComponent>(enemy2, {.hurtBox = {0.f, 0.f, 20.f, 20.f}});
	}
};

TEST_F(HitDetectionMultiTargetTest, AttackerShouldHitMultipleEnemies)
{
	auto view = registry.view<AttackComponent>();
	auto &attack = view.get<AttackComponent>(attacker);
	attack.attackTimer.reset();
	attack.attackTimer.step(std::chrono::duration<float>(0.1f), false);

	bool callbackCalled = false;
	GameEngine::ECS::Entity calledEntity{};
	auto mockCallback = [&](GameEngine::ECS::Entity e1, GameEngine::ECS::Entity e2) { callbackCalled = true; };

	// Both enemies are in range.
	// Update HitDetection
	GameEngine::ECS::Systems::HitDetectionSystem::update(registry, mockCallback);

	// Both should have PendingDamage
	bool e1Hit = registry.has<PendingDamageComponent>(enemy1);
	bool e2Hit = registry.has<PendingDamageComponent>(enemy2);

	EXPECT_TRUE(e1Hit) << "Enemy 1 should be hit";
	EXPECT_TRUE(e2Hit) << "Enemy 2 should be hit";
}
