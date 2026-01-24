#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/JumpSystem.hpp"
#include "Engine/GameEngine.hpp"

#include <gtest/gtest.h>

using namespace GameEngine::ECS;
using namespace GameEngine::ECS::Systems;
using namespace std::chrono_literals;

constexpr std::chrono::duration<float> coyoteThreshold = 0.08s;

struct JumpSystemTest : testing::Test {
	Registry registry{};
	Entity entity{};

	void SetUp() override
	{
		entity = registry.create();

		registry.registerComponent<JumpComponent>(entity,
		                                          JumpComponent{.maxJumps = 2, .coyoteTimer = Timer{coyoteThreshold}});
		registry.registerComponent<GroundedComponent>(entity, GroundedComponent{.grounded = true});
		registry.registerComponent<VelocityComponent>(entity, VelocityComponent{});
		registry.registerComponent<InputComponent>(entity, InputComponent{});
	}

	JumpComponent &getJump() { return registry.view<JumpComponent>().get<JumpComponent>(entity); }
	GroundedComponent &getGrounded() { return registry.view<GroundedComponent>().get<GroundedComponent>(entity); }
	VelocityComponent &getVelocity() { return registry.view<VelocityComponent>().get<VelocityComponent>(entity); }
	InputComponent &getInput() { return registry.view<InputComponent>().get<InputComponent>(entity); }

	void triggerJumpInput() { getInput().jumpPressed = true; }
	void setGrounded(const bool grounded) { getGrounded().grounded = grounded; }
	void setJumpsLeft(const int jumpsLeft) { getJump().jumpsLeft = jumpsLeft; }
	void setMaxJumps(const int maxJumps) { getJump().maxJumps = maxJumps; }
	void setUpSingleJump()
	{
		setMaxJumps(1);
		setJumpsLeft(1);
	}
	void setUpDoubleJump()
	{
		setMaxJumps(2);
		setJumpsLeft(2);
	}
};

TEST_F(JumpSystemTest, YVelocityAfterJumpInput)
{
	const auto &[velocity] = getVelocity();
	setUpSingleJump();

	EXPECT_FLOAT_EQ(velocity.y, 0.0f);

	triggerJumpInput();
	JumpSystem::update(registry, 0.1f);

	EXPECT_FLOAT_EQ(velocity.y, -getJump().jumpStrength);
}

TEST_F(JumpSystemTest, JumpsLeftAfterSingleJump)
{
	setUpDoubleJump();

	JumpSystem::update(registry, 0.1f);
	EXPECT_EQ(getJump().jumpsLeft, 2);

	triggerJumpInput();
	JumpSystem::update(registry, 0.1f);
	EXPECT_EQ(getJump().jumpsLeft, 1);
}

TEST_F(JumpSystemTest, JumpsLeftAfterDoubleJump)
{
	setUpDoubleJump();

	JumpSystem::update(registry, 0.1f);
	EXPECT_EQ(getJump().jumpsLeft, 2);

	triggerJumpInput();
	JumpSystem::update(registry, 0.1f);
	EXPECT_EQ(getJump().jumpsLeft, 1);
	setGrounded(false);

	triggerJumpInput();
	JumpSystem::update(registry, 0.1f);
	EXPECT_EQ(getJump().jumpsLeft, 0);
}

TEST_F(JumpSystemTest, StepOffTileBelowCoyoteThresholdSingleJump)
{
	setUpSingleJump();
	setGrounded(false);

	JumpSystem::update(registry, (coyoteThreshold - 0.01s).count());

	EXPECT_EQ(getJump().jumpsLeft, 1);
	EXPECT_FLOAT_EQ(getVelocity().velocity.y, 0.0f);
}

TEST_F(JumpSystemTest, StepOffTileAboveCoyoteThresholSingleJump)
{
	setUpSingleJump();
	setGrounded(false);

	JumpSystem::update(registry, (coyoteThreshold + 0.01s).count());

	EXPECT_EQ(getJump().jumpsLeft, 0);
	EXPECT_FLOAT_EQ(getVelocity().velocity.y, 0.0f);
}

TEST_F(JumpSystemTest, StepOffTileBelowCoyoteThresholdDoubleJump)
{
	setUpDoubleJump();
	setGrounded(false);

	JumpSystem::update(registry, (coyoteThreshold - 0.01s).count());

	EXPECT_EQ(getJump().jumpsLeft, 2);
	EXPECT_FLOAT_EQ(getVelocity().velocity.y, 0.0f);
}

TEST_F(JumpSystemTest, StepOffTileAboveCoyoteThresholDoubleJump)
{
	setUpDoubleJump();
	setGrounded(false);

	JumpSystem::update(registry, (coyoteThreshold + 0.01s).count());

	EXPECT_EQ(getJump().jumpsLeft, 1);
	EXPECT_FLOAT_EQ(getVelocity().velocity.y, 0.0f);
}
