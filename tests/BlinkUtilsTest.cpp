#include "Engine/Utils/BlinkUtils.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include <gtest/gtest.h>

using namespace GameEngine::ECS;
using namespace GameEngine::Utils;

class BlinkUtilsTest : public ::testing::Test {
  protected:
	Registry registry;
	Entity entity{};

	void SetUp() override
	{
		entity = registry.create();
		registry.registerComponent<HealthComponent>(entity, {20, 20, std::chrono::duration<float>(1.0f)});
		registry.registerComponent<BlinkComponent>(entity, {{255, 0, 0, 255}, 0.1f});
	}
};

TEST_F(BlinkUtilsTest, ReturnsWhiteWhenNotInvulnerable)
{
	auto color = Blink::getBlinkColor(registry, entity);
	EXPECT_EQ(color.r, 255);
	EXPECT_EQ(color.g, 255);
	EXPECT_EQ(color.b, 255);
}

TEST_F(BlinkUtilsTest, ReturnsBlinkColorDuringInvulnerability)
{
	auto &health = registry.get<HealthComponent>(entity);
	health.invulnerabilityTimer.reset();
	health.invulnerabilityTimer.step(std::chrono::duration<float>(0.1f), false);

	auto color = Blink::getBlinkColor(registry, entity);
	EXPECT_EQ(color.r, 255);
	EXPECT_EQ(color.g, 0);
	EXPECT_EQ(color.b, 0);
}

TEST_F(BlinkUtilsTest, TogglesColorBasedOnFrequency)
{
	auto &health = registry.get<HealthComponent>(entity);
	health.invulnerabilityTimer.reset();
	health.invulnerabilityTimer.step(std::chrono::duration<float>(0.15f), false);

	auto color = Blink::getBlinkColor(registry, entity);
	EXPECT_EQ(color.r, 255);
	EXPECT_EQ(color.g, 0);
	EXPECT_EQ(color.b, 0);
}
