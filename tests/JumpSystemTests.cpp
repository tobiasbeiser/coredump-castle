#include "Engine/GameEngine.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/JumpSystem.hpp"

#include <gtest/gtest.h>

using namespace GameEngine::ECS;
using namespace GameEngine::ECS::Systems;


class JumpSystemTest : public ::testing::Test {
  protected:
	Registry registry;
	Entity entity;

	void SetUp() override
	{
		entity = registry.create();

		registry.registerComponent<JumpComponent>(entity, JumpComponent {.maxJumps = 2, .coyoteTimer = 0.15f});
		registry.registerComponent<GroundedComponent>(entity, GroundedComponent{.grounded = true});
		registry.registerComponent<VelocityComponent>(entity, VelocityComponent{});
		registry.registerComponent<InputComponent>(entity, InputComponent{});
	}

	JumpComponent &getJump() { return registry.get<JumpComponent>(entity); }
	GroundedComponent &getGrounded() { return registry.get<GroundedComponent>(entity); }
	VelocityComponent &getVelocity() { return registry.get<VelocityComponent>(entity); }
	InputComponent &getInput() { return registry.get<InputComponent>(entity); }

	void triggerJumpInput() { getInput().jumpPressed = true; }
	void setGrounded(const bool grounded) { getGrounded().grounded = grounded; }
	void setJumpsLeft(const int jumpsLeft) { getJump().jumpsLeft = jumpsLeft; }
	void setMaxJumps(const int maxJumps) { getJump().maxJumps = maxJumps; }
};

TEST_F(JumpSystemTest, YVelocityAfterJumpInput)
{
	const auto &[velocity] = getVelocity();

	setGrounded(true);

	EXPECT_FLOAT_EQ(velocity.y, 0.0f);

	triggerJumpInput();
	JumpSystem::update(registry, 0.1f);

	EXPECT_FLOAT_EQ(velocity.y, -getJump().jumpStrength);
}

TEST_F(JumpSystemTest, JumpsLeftAfterJumpInput)
{

	setGrounded(true);
	JumpSystem::update(registry, 0.1f);

	EXPECT_EQ(getJump().jumpsLeft, 2);

	triggerJumpInput();
	JumpSystem::update(registry, 0.1f);

	EXPECT_EQ(getJump().jumpsLeft, 1);
}

TEST_F(JumpSystemTest, JumpsLeftAfterDoubleJumpInput)
{

	setGrounded(true);
	JumpSystem::update(registry, 0.1f);

	EXPECT_EQ(getJump().jumpsLeft, 2);

	triggerJumpInput();
	JumpSystem::update(registry, 0.1f);

	EXPECT_EQ(getJump().jumpsLeft, 1);

	triggerJumpInput();
	JumpSystem::update(registry, 0.1f);

	EXPECT_EQ(getJump().jumpsLeft, 0);
}

TEST_F(JumpSystemTest, StepOffTileOneJumpLeft)
{
	EXPECT_TRUE(getGrounded().grounded);
	setGrounded(false);
	setJumpsLeft(2);

	JumpSystem::update(registry, 0.1f);

	EXPECT_EQ(getJump().jumpsLeft, 1);
	EXPECT_FLOAT_EQ(getVelocity().velocity.y, 0.0f);
}

TEST_F(JumpSystemTest, NoJumpAfterCoyoteTimer)
{
	EXPECT_TRUE(getGrounded().grounded);
	EXPECT_FLOAT_EQ(getVelocity().velocity.y, 0);
	setGrounded(false);
	setJumpsLeft(2);
	EXPECT_FALSE(getGrounded().grounded);

	JumpSystem::update(registry, 0.2f);
	EXPECT_EQ(getJump().jumpsLeft, 0);
	EXPECT_FLOAT_EQ(getVelocity().velocity.y, 0);
}
