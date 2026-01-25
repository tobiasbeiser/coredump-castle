#include "Engine/ECS/Systems/Combat/PoiseSystem.hpp"

#include "Engine/ECS/Components/Gameplay/Combat/PoiseComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include <gtest/gtest.h>

TEST(PoiseSystemTest, ResetsPoiseAfterRecovery)
{
	GameEngine::ECS::Registry registry;
	const auto entity = registry.create();
	registry.registerComponent(entity,
	                           PoiseComponent{.currentPoise = 5.0f,
	                                          .maxPoise = 10.0f,
	                                          .recoveryTimer = Timer{std::chrono::duration<float>(0.1f)}});

	GameEngine::ECS::Systems::PoiseSystem::update(registry, 0.2f);

	auto view = registry.view<PoiseComponent>();
	EXPECT_FLOAT_EQ(view.get<PoiseComponent>(entity).currentPoise, 0.0f);
}

TEST(PoiseSystemTest, DoesNotResetBeforeTimerCompletes)
{
	GameEngine::ECS::Registry registry;
	const auto entity = registry.create();
	registry.registerComponent(entity,
	                           PoiseComponent{.currentPoise = 5.0f,
	                                          .maxPoise = 5.0f,
	                                          .recoveryTimer = Timer{std::chrono::duration<float>(1.0f)}});

	GameEngine::ECS::Systems::PoiseSystem::update(registry, 0.5f);

	auto view = registry.view<PoiseComponent>();
	EXPECT_FLOAT_EQ(view.get<PoiseComponent>(entity).currentPoise, 5.0f);
}

TEST(PoiseSystemTest, DoesNothingWhenPoiseIsZero)
{
	GameEngine::ECS::Registry registry;
	const auto entity = registry.create();
	registry.registerComponent(entity,
	                           PoiseComponent{.currentPoise = 0.0f,
	                                          .maxPoise = 0.0f,
	                                          .recoveryTimer = Timer{std::chrono::duration<float>(0.1f)}});

	GameEngine::ECS::Systems::PoiseSystem::update(registry, 1.0f);

	auto view = registry.view<PoiseComponent>();
	EXPECT_FLOAT_EQ(view.get<PoiseComponent>(entity).currentPoise, 0.0f);
}
