#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/StateMachine/StateMachineComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/StateMachineSystem.hpp"
#include "Game/AI/StateMachine/EnemyStateMachineFactory.hpp"
#include "Game/AI/States.hpp"
#include <gtest/gtest.h>

using namespace GameEngine::ECS;
using namespace GameEngine::AI;

struct ArcherBossBehaviorTest : testing::Test {
	Registry registry{};
	Entity entity{};
	StateMachineComponent *sm = nullptr;

	void SetUp() override
	{
		entity = registry.create();
		registry.registerComponent(entity, PatrolComponent{});
		registry.registerComponent(
		    entity, ChaseComponent{.chaseSpeed = 100.0f, .detectionRange = 200.0f, .giveUpRange = 300.0f});
		registry.registerComponent(entity, VelocityComponent{});
		registry.registerComponent(entity, MovementComponent{});
		registry.registerComponent(entity, PositionComponent{});
		registry.registerComponent(entity, GroundedComponent{.grounded = true});
		registry.registerComponent(entity, RenderComponent{.alpha = 255});
		registry.registerComponent(entity, PatrolComponent{});
		registry.registerComponent(entity, InputComponent{});
		registry.registerComponent(entity, HitBoxComponent{});
		registry.registerComponent(entity, HurtBoxComponent{});

		registry.registerComponent(entity, JumpComponent{.jumpStrength = 400.0f, .jumpsLeft = 1, .maxJumps = 1});

		EnemyStateMachineFactory::createArcherBoss(registry, entity);
		sm = &registry.get<StateMachineComponent>(entity);
	}
};

TEST_F(ArcherBossBehaviorTest, TransitionsToCorneredFireWhenStuck)
{
	sm->currentState = AIState::Retreat;
	sm->timeInState = 0.0f;

	const auto player = registry.create();
	registry.registerComponent(player, PlayerTag{});
	registry.registerComponent(player, PositionComponent{.position = {0.0f, 0.0f}});
	registry.registerComponent(player, HurtBoxComponent{});

	registry.get<VelocityComponent>(entity).velocity = {0.0f, 0.0f};

	Systems::StateMachineSystem::update(registry, 0.3f);

	EXPECT_EQ(sm->currentState, AIState::CorneredFire);
}

TEST_F(ArcherBossBehaviorTest, TransitionsToTelegraphEscapeWhenPlayerClose)
{
	sm->currentState = AIState::CorneredFire;
	sm->timeInState = 0.0f;

	const auto player = registry.create();
	registry.registerComponent(player, PlayerTag{});
	registry.registerComponent(player, PositionComponent{.position = {0.0f, 0.0f}});
	registry.registerComponent(player, TransformComponent{});
	registry.registerComponent(player, HitBoxComponent{});
	registry.registerComponent(player, HurtBoxComponent{});

	registry.get<PositionComponent>(entity).position = {10.0f, 0.0f}; // Very close (10 units)

	Systems::StateMachineSystem::update(registry, 0.1f);

	EXPECT_EQ(sm->currentState, AIState::TelegraphCharge);
}

TEST_F(ArcherBossBehaviorTest, PhaseShiftLowersOpacity)
{
	registry.get<PatrolComponent>(entity).patrolCenter = {1000.0f, 0.0f};

	registry.get<PositionComponent>(entity).position = {0.0f, 0.0f};

	const auto player = registry.create();
	registry.registerComponent(player, PlayerTag{});
	registry.registerComponent(player, PositionComponent{.position = {200.0f, 0.0f}});
	registry.registerComponent(player, HurtBoxComponent{});

	sm->currentState = AIState::TelegraphCharge;
	sm->timeInState = 0.0f;
	sm->justEnteredState = false;

	Systems::StateMachineSystem::update(registry, 1.3f);

	EXPECT_EQ(sm->currentState, AIState::Escaping);

	Systems::StateMachineSystem::update(registry, 0.1f);

	const auto &render = registry.get<RenderComponent>(entity);
	EXPECT_EQ(render.alpha, 128);

	EXPECT_EQ(sm->currentState, AIState::Escaping);
}

TEST_F(ArcherBossBehaviorTest, PhaseShiftRestoresOpacityCheck)
{
	registry.get<PatrolComponent>(entity).patrolCenter = {1000.0f, 0.0f};

	const auto player = registry.create();
	registry.registerComponent(player, PlayerTag{});
	registry.registerComponent(player, PositionComponent{.position = {100.0f, 0.0f}});
	registry.registerComponent(player, HurtBoxComponent{});

	sm->currentState = AIState::Escaping;
	sm->timeInState = 0.0f;
	registry.get<RenderComponent>(entity).alpha = 128;

	registry.get<PositionComponent>(entity).position = {300.0f, 0.0f};

	Systems::StateMachineSystem::update(registry, 0.1f);

	EXPECT_EQ(sm->currentState, AIState::Retreat);

	const auto &render = registry.get<RenderComponent>(entity);
	EXPECT_EQ(render.alpha, 255);
}

TEST_F(ArcherBossBehaviorTest, EscapesTowardsPatrolCenter)
{
	registry.get<PatrolComponent>(entity).patrolCenter = {500.0f, 0.0f};

	registry.get<PositionComponent>(entity).position = {100.0f, 0.0f};

	const auto player = registry.create();
	registry.registerComponent(player, PlayerTag{});
	registry.registerComponent(player, PositionComponent{.position = {150.0f, 0.0f}});
	registry.registerComponent(player, TransformComponent{});
	registry.registerComponent(player, HitBoxComponent{});
	registry.registerComponent(player, HurtBoxComponent{});

	sm->currentState = AIState::Escaping;
	sm->timeInState = 0.0f;

	registry.get<InputComponent>(entity).horizontalDirection = 0.0f;
	registry.get<MovementComponent>(entity).maxSpeedX = 0.0f;

	Systems::StateMachineSystem::update(registry, 0.1f);

	const auto &vel = registry.get<VelocityComponent>(entity);
	EXPECT_GT(vel.velocity.x, 0.0f);
	EXPECT_EQ(registry.get<InputComponent>(entity).horizontalDirection, 1.0f);
}
