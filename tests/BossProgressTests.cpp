

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/BossComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/ProgressSystem.hpp"

#include <Engine/ECS/View.hpp>

using namespace std::chrono_literals;

class MockSaveSystem {
public:
	MOCK_CONST_METHOD1(saveGame, void(GameEngine::ECS::Registry& reg));
};

struct BossProgressTests : testing::Test {
	GameEngine::ECS::Registry registry{};
	GameEngine::ECS::Entity player{};
	GameEngine::ECS::Entity boss{};

	MockSaveSystem mockSaveSystem;
	void SetUp() override
	{
		player = registry.create();
		registry.registerComponent<PlayerTag>(player, PlayerTag{});
		registry.registerComponent<ProgressComponent>(player, ProgressComponent{});
		registry.registerComponent<JumpComponent>(player, JumpComponent{
		                                                      .jumpsLeft = 1,
		                                                      .maxJumps = 1,
		                                                  });
		registry.registerComponent<StateComponent>(player, StateComponent{});
		registry.registerComponent<PositionComponent>(player, PositionComponent{});
		registry.registerComponent<>(player, PositionComponent{});
		registry.registerComponent<>(player, InteractionComponent{});

		boss = registry.create();
		registry.registerComponent<BossComponent>(
		    boss, BossComponent{.bossId = 42, .unlockAbility = AbilityType::DoubleJump});
		registry.registerComponent<HealthComponent>(boss, HealthComponent{0, 10, 0ms});
	}
};

TEST_F(BossProgressTests, DefeatingBossUnlocksDoubleJump)
{

	EXPECT_CALL(mockSaveSystem, saveGame(testing::_));
	GameEngine::ECS::Systems::ProgressSystem::update(registry, mockSaveSystem);

	auto playerView = registry.view<PlayerTag, ProgressComponent, JumpComponent, StateComponent>();
	const auto entity = playerView.at(0U);
	const auto &progress = playerView.get<ProgressComponent>(entity);
	const auto &jump = playerView.get<JumpComponent>(entity);
	const auto &state = playerView.get<StateComponent>(entity);

	EXPECT_TRUE(progress.isBossDefeated(42));
	EXPECT_TRUE(progress.hasAbility(AbilityType::DoubleJump));
	EXPECT_EQ(2, jump.maxJumps);
	EXPECT_EQ(2, jump.jumpsLeft);
	EXPECT_EQ(State::Interacting, state.state);
}
