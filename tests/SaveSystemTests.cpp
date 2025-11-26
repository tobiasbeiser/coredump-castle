
#include <gtest/gtest.h>
#include <vector>

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/SaveSystem.hpp"

#include <SDL3/SDL_stdinc.h>

namespace {

using GameEngine::ECS::Entity;
using GameEngine::ECS::ISaveStorage;
using GameEngine::ECS::Registry;
using GameEngine::ECS::Systems::SaveSystem;
using json = nlohmann::json;

class FakeSaveStorage final : public ISaveStorage {
  public:
	void write(const json &data, const std::string &fileName) override
	{
		hasData = true;
		store[fileName] = data;
	}

	[[nodiscard]] json read(const std::string &fileName) const override
	{
		const auto it = store.find(fileName);
		if (it == store.end()) {
			throw std::runtime_error("No data written for file " + fileName);
		}
		return it->second;
	}

  private:
	std::unordered_map<std::string, json> store{};
	bool hasData{false};
};

class SaveSystemTest : public ::testing::Test {
  public:
	Registry registry;
	Entity playerEntity{};

  protected:
	void SetUp() override
	{
		playerEntity = registry.create();
		registry.registerComponent<PlayerTag>(playerEntity, PlayerTag{});

		JumpComponent jumpComponent{.maxJumps = 2, .coyoteTimer = 28};
		registry.registerComponent<JumpComponent>(playerEntity, jumpComponent);

		PositionComponent positionComponent{};
		positionComponent.position = glm::vec2{1.0F, 3.3F};
		registry.registerComponent<PositionComponent>(playerEntity, positionComponent);

		InventoryComponent inventoryComponent{};
		inventoryComponent.keys = std::vector<int>{9, 4, 5, 2, 1, 8};
		registry.registerComponent<InventoryComponent>(playerEntity, inventoryComponent);
	}
};

TEST_F(SaveSystemTest, SaveGame_ProducesExpectedJson)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	const auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	const std::string fileName{"SaveGame.json"};
	saveSystem.saveGame(registry, fileName);

	const json data = storagePtr->read(fileName);

	EXPECT_EQ(2, data.at("maxJumps").get<int>());
	EXPECT_FLOAT_EQ(1.0F, data.at("positionX").get<float>());
	EXPECT_FLOAT_EQ(3.3F, data.at("positionY").get<float>());

	const auto keysFromFile = data.at("keys").get<std::vector<int>>();
	const std::vector<int> expectedKeys{9, 4, 5, 2, 1, 8};
	EXPECT_EQ(expectedKeys, keysFromFile);
}

TEST_F(SaveSystemTest, LoadSavedGame_RestoresPlayerStateFromJson)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	const std::string fileName{"SaveGame.json"};

	json saved;
	saved["maxJumps"] = 7;
	saved["positionX"] = 10.5F;
	saved["positionY"] = -3.0F;
	saved["keys"] = std::vector<int>{1, 2, 3};

	storagePtr->write(saved, fileName);

	auto playerView = registry.view<PlayerTag>();
	const Entity entity = playerView.at(0U);

	auto &jumpComponent = playerView.get<JumpComponent>(entity);
	jumpComponent.maxJumps = 0;

	auto &positionComponent = playerView.get<PositionComponent>(entity);
	positionComponent.position = glm::vec2{0.0F, 0.0F};

	auto &inventoryComponent = playerView.get<InventoryComponent>(entity);
	inventoryComponent.keys.clear();

	saveSystem.loadSavedGame(registry, fileName);

	const auto &jumpComponentAfter = playerView.get<JumpComponent>(entity);
	EXPECT_EQ(7, jumpComponentAfter.maxJumps);

	const auto &positionComponentAfter = playerView.get<PositionComponent>(entity);
	EXPECT_FLOAT_EQ(10.5F, positionComponentAfter.position.x);
	EXPECT_FLOAT_EQ(-3.0F, positionComponentAfter.position.y);

	const auto &inventoryComponentAfter = playerView.get<InventoryComponent>(entity);
	const std::vector<int> expectedKeys{1, 2, 3};
	EXPECT_EQ(expectedKeys, inventoryComponentAfter.keys);
}

TEST_F(SaveSystemTest, LoadSavedGame_RemovesCollectedLocksAndKeys)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	const std::string fileName{"SaveGame.json"};

	json saved;
	saved["maxJumps"] = 2;
	saved["positionX"] = 1.0F;
	saved["positionY"] = 3.3F;
	saved["keys"] = std::vector<int>{9, 4, 5, 2, 1, 8};

	storagePtr->write(saved, fileName);

	const Entity lock1 = registry.create();
	registry.registerComponent<LockComponent>(lock1, LockComponent{.lockId = 4});

	const Entity lock2 = registry.create();
	registry.registerComponent<LockComponent>(lock2, LockComponent{.lockId = 99});

	const Entity key1 = registry.create();
	registry.registerComponent<KeyComponent>(key1, KeyComponent{.id = 1});

	const Entity key2 = registry.create();
	registry.registerComponent<KeyComponent>(key2, KeyComponent{.id = 42});

	saveSystem.loadSavedGame(registry, fileName);

	auto lockView = registry.view<LockComponent>();
	std::vector<int> remainingLocks;
	remainingLocks.reserve(lockView.size());
	for (const Entity e : lockView) {
		const auto &lockComponent = lockView.get<LockComponent>(e);
		remainingLocks.push_back(lockComponent.lockId);
	}

	ASSERT_EQ(1U, remainingLocks.size());
	EXPECT_EQ(99, remainingLocks.front());

	auto keyView = registry.view<KeyComponent>();
	std::vector<int> remainingKeys;
	remainingKeys.reserve(keyView.size());
	for (const Entity e : keyView) {
		const auto &keyComponent = keyView.get<KeyComponent>(e);
		remainingKeys.push_back(keyComponent.id);
	}

	ASSERT_EQ(1U, remainingKeys.size());
	EXPECT_EQ(42, remainingKeys.front());
}

TEST_F(SaveSystemTest, SaveGame_ThrowsIfMultiplePlayerEntitiesExist)
{
	const Entity secondPlayer = registry.create();
	registry.registerComponent<PlayerTag>(secondPlayer, PlayerTag{});

	auto storage = std::make_unique<FakeSaveStorage>();
	const SaveSystem saveSystem{std::move(storage)};

	const std::string fileName{"SaveGame.json"};
	EXPECT_THROW(saveSystem.saveGame(registry, fileName), std::runtime_error);
}

TEST_F(SaveSystemTest, LoadSavedGame_ThrowsIfMultiplePlayerEntitiesExist)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	const std::string fileName{"SaveGame.json"};

	json saved;
	saved["maxJumps"] = 2;
	saved["positionX"] = 1.0F;
	saved["positionY"] = 3.3F;
	saved["keys"] = std::vector<int>{};
	storagePtr->write(saved, fileName);

	const Entity secondPlayer = registry.create();
	registry.registerComponent<PlayerTag>(secondPlayer, PlayerTag{});

	EXPECT_THROW(saveSystem.loadSavedGame(registry, fileName), std::runtime_error);
}

} // namespace
