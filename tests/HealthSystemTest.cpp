#include "Engine/ECS/Systems/Combat/HealthSystem.hpp"

#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Entity.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/StateSystem.hpp"
#include "Engine/ECS/View.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;

struct HealthSystemTest : testing::Test {
	GameEngine::ECS::Registry registry{};
	GameEngine::ECS::Entity entity{};

	void SetUp() override
	{
		entity = registry.create();
		registry.registerComponent<HealthComponent>(entity, {20, 20, 1s});
		registry.registerComponent<StateComponent>(entity, {});
		registry.registerComponent<AnimationComponent>(entity, {});

		AnimationComponent animComp;
		animComp.addAnimation(AnimationType::Dead, {}, {}, std::chrono::duration<float>(1.0f));
		registry.registerComponent(entity, animComp);
		animComp.addAnimation(AnimationType::Hurting, {}, {}, std::chrono::duration<float>(1.0f));
		registry.registerComponent(entity, animComp);
	}
};

TEST_F(HealthSystemTest, NoDamageRecivedShouldRemainInIdleState)
{
	auto view = registry.view<StateComponent>();
	const auto &state = view.get<StateComponent>(entity);

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);

	EXPECT_EQ(State::Idle, state.state);
	EXPECT_FALSE(registry.has<PendingDamageComponent>(entity));
}

TEST_F(HealthSystemTest, ReceivedDamageShouldApplyDamage)
{
	auto view = registry.view<StateComponent, HealthComponent>();
	const auto &health = view.get<HealthComponent>(entity);
	const auto &state = view.get<StateComponent>(entity);
	registry.registerComponent(entity, PendingDamageComponent{10});

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);
	GameEngine::ECS::Systems::StateSystem::update(registry);

	EXPECT_EQ(health.health, 10);
	EXPECT_EQ(State::Hurting, state.state);
}

TEST_F(HealthSystemTest, DamageIsOnlyProcessedOnce)
{
	auto view = registry.view<HealthComponent>();
	const auto &health = view.get<HealthComponent>(entity);
	registry.registerComponent(entity, PendingDamageComponent{10});

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);
	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);
	EXPECT_EQ(health.health, 10);
}

TEST_F(HealthSystemTest, ZeroDamageShouldNotChangeState)
{
	auto view = registry.view<HealthComponent>();
	const auto &health = view.get<HealthComponent>(entity);

	registry.registerComponent(entity, PendingDamageComponent{});
	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);

	EXPECT_EQ(health.health, 20);
}

TEST_F(HealthSystemTest, NoHealthRemainShouldDie)
{
	auto view = registry.view<HealthComponent, StateComponent>();
	const auto &state = view.get<StateComponent>(entity);
	const auto &health = view.get<HealthComponent>(entity);

	registry.registerComponent(entity, PendingDamageComponent{20});

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);
	GameEngine::ECS::Systems::StateSystem::update(registry);

	EXPECT_EQ(health.health, 0);
	EXPECT_EQ(State::Dead, state.state);
}

TEST_F(HealthSystemTest, DamageWhileInvulnerableShouldNotApply)
{
	auto view = registry.view<HealthComponent, StateComponent>();
	const auto &state = view.get<StateComponent>(entity);
	const auto &health = view.get<HealthComponent>(entity);

	registry.registerComponent(entity, PendingDamageComponent{10});

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);
	GameEngine::ECS::Systems::StateSystem::update(registry);

	registry.registerComponent(entity, PendingDamageComponent{10});

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);
	GameEngine::ECS::Systems::StateSystem::update(registry);

	EXPECT_EQ(health.health, 10);
	EXPECT_EQ(health.invulnerabilityTimer.getTime(), std::chrono::duration<float>(0.1f));
	EXPECT_EQ(State::Hurting, state.state);
}

TEST_F(HealthSystemTest, DamageExceedingHealthShouldClampToZero)
{
	auto view = registry.view<HealthComponent, StateComponent>();
	const auto &state = view.get<StateComponent>(entity);
	const auto &health = view.get<HealthComponent>(entity);

	registry.registerComponent(entity, PendingDamageComponent{30});

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);
	GameEngine::ECS::Systems::StateSystem::update(registry);

	EXPECT_EQ(health.health, 0);
	EXPECT_EQ(State::Dead, state.state);
}

TEST_F(HealthSystemTest, InvulnerabilityTimerShouldResetAfterInitialDamage)
{
	auto view = registry.view<HealthComponent>();
	const auto &health = view.get<HealthComponent>(entity);

	registry.registerComponent(entity, PendingDamageComponent{10});

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);

	EXPECT_EQ(health.invulnerabilityTimer.getTime(), std::chrono::duration<float>(0.0f));
}

TEST_F(HealthSystemTest, EntityWithoutAnimationShouldStillTakeDamage)
{
	auto view = registry.view<HealthComponent>();
	const auto &health = view.get<HealthComponent>(entity);

	registry.registerComponent(entity, PendingDamageComponent{10});
	registry.remove<AnimationComponent>(entity);

	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1);

	EXPECT_EQ(health.health, 10);
}

TEST_F(HealthSystemTest, InvulnerabilityTimerShouldExpireAfterDuration)
{
	auto view = registry.view<HealthComponent>();
	const auto &health = view.get<HealthComponent>(entity);

	registry.registerComponent(entity, PendingDamageComponent{10});
	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1f);

	GameEngine::ECS::Systems::HealthSystem::update(registry, 1.0f);
	GameEngine::ECS::Systems::StateSystem::update(registry);

	registry.registerComponent(entity, PendingDamageComponent{10});
	GameEngine::ECS::Systems::HealthSystem::update(registry, 0.1f);

	EXPECT_EQ(health.health, 0);
}
