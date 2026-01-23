#include "Engine/ECS/Systems/BlinkSystem.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

#include <gtest/gtest.h>

using namespace GameEngine::ECS;

struct BlinkSystemTest : testing::Test {
	Registry registry;
	Entity entity{};

	void SetUp() override
	{
		entity = registry.create();
		registry.registerComponent<HealthComponent>(entity, {20, 20, std::chrono::duration<float>(1.0f)});
		registry.registerComponent<SpriteComponent>(entity, {});
		registry.registerComponent<BlinkComponent>(entity, {{255, 0, 0, 255}, 0.1f});
	}
};

TEST_F(BlinkSystemTest, ReturnsWhiteWhenNotInvulnerable)
{
	auto view = registry.view<SpriteComponent>();
	const auto &color = view.get<SpriteComponent>(entity).colorMod;

	EXPECT_EQ(color.r, 255);
	EXPECT_EQ(color.g, 255);
	EXPECT_EQ(color.b, 255);

	Systems::BlinkSystem::update(registry);

	EXPECT_EQ(color.r, 255);
	EXPECT_EQ(color.g, 255);
	EXPECT_EQ(color.b, 255);
}

TEST_F(BlinkSystemTest, ReturnsBlinkColorDuringInvulnerability)
{
	auto view = registry.view<SpriteComponent, HealthComponent>();
	auto &health = view.get<HealthComponent>(entity);
	health.invulnerabilityTimer.reset();
	health.invulnerabilityTimer.step(std::chrono::duration<float>::zero(), false);

	auto &color = view.get<SpriteComponent>(entity).colorMod;

	EXPECT_EQ(color.r, 255);
	EXPECT_EQ(color.g, 255);
	EXPECT_EQ(color.b, 255);

	Systems::BlinkSystem::update(registry);

	EXPECT_EQ(color.r, 255);
	EXPECT_EQ(color.g, 0);
	EXPECT_EQ(color.b, 0);
}

TEST_F(BlinkSystemTest, TogglesColorBasedOnFrequency)
{
	auto view = registry.view<SpriteComponent, HealthComponent>();

	auto &health = view.get<HealthComponent>(entity);
	health.invulnerabilityTimer.reset();
	health.invulnerabilityTimer.step(std::chrono::duration<float>::zero(), false);

	auto &color = view.get<SpriteComponent>(entity).colorMod;

	EXPECT_EQ(color.r, 255);
	EXPECT_EQ(color.g, 255);
	EXPECT_EQ(color.b, 255);

	Systems::BlinkSystem::update(registry);

	EXPECT_EQ(color.r, 255);
	EXPECT_EQ(color.g, 0);
	EXPECT_EQ(color.b, 0);
}
