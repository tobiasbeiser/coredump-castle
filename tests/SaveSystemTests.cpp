#include "Engine/GameEngine.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Systems/JumpSystem.hpp"
#include "Engine/ECS/Systems/SaveSystem.hpp"

#include <gtest/gtest.h>

using namespace GameEngine::ECS;
using namespace GameEngine::ECS::Systems;


class SaveSystemTest: public ::testing::Test {
  protected:
	Registry registry;
	Entity entity;
	std::filesystem::path fullPath = std::filesystem::current_path() / "savefiles" ;

	void SetUp() override
	{
		SaveSystem::removeSavedGame();
		entity = registry.create();
		registry.registerComponent<PlayerTag>(entity, PlayerTag{});
		registry.registerComponent<JumpComponent>(entity, JumpComponent {.maxJumps = 2, .coyoteTimer = 28});
		registry.registerComponent<PositionComponent>(entity, PositionComponent{.position = glm::vec2(1.0f, 3.3f)});
		registry.registerComponent<InventoryComponent>(entity, InventoryComponent{std::vector{9, 4, 5, 2, 1, 8}});

	}

};

TEST_F(SaveSystemTest, TestPathCreation)
{
	std::filesystem::path fullPath = std::filesystem::current_path() / "savefiles" ;
	EXPECT_FALSE(std::filesystem::exists(fullPath));
	SaveSystem::saveGame(registry);
	EXPECT_TRUE(std::filesystem::exists(fullPath));
	const json savedFile(fullPath);
	std::cout << savedFile << std::endl;
}
