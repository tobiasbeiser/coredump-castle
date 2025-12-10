
#include <gtest/gtest.h>
#include <vector>

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/SaveSystem.hpp"

namespace {

using GameEngine::ECS::Entity;
using GameEngine::ECS::ISaveStorage;
using GameEngine::ECS::Registry;
using GameEngine::ECS::Systems::SaveSystem;
using json = nlohmann::json;

class FakeSaveStorage final : public ISaveStorage {
  public:
	void write(const json &data) override { store = data; }

	[[nodiscard]] json read() const override { return store; }

	[[nodiscard]] std::vector<std::string> getSavesNames() const override { return existingSaveNames; }

	void setSaveName(std::string saveName) override { currentSaveName = std::move(saveName); }

	[[nodiscard]] const json &getStoredData() const { return store; }

	// Helper to inspect the currently active save name
	[[nodiscard]] const std::string &getCurrentSaveName() const { return currentSaveName; }

	// Helper to inject fake file existence
	void setExistingSaveNames(std::vector<std::string> names) { existingSaveNames = std::move(names); }

  private:
	json store{};
	std::string currentSaveName;
	std::vector<std::string> existingSaveNames;
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

	saveSystem.saveGame(registry);

	const json data = storagePtr->read();

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

	// 1. Setup the fake data
	json saved;
	saved["maxJumps"] = 7;
	saved["positionX"] = 10.5F;
	saved["positionY"] = -3.0F;
	saved["keys"] = std::vector<int>{1, 2, 3};
	storagePtr->write(saved);

	// 2. CRITICAL: Register the filename in the fake storage.
	// If we don't do this, the system sees the file as missing and overwrites
	// the 'saved' data with empty JSON.
	const std::string saveName = "Level1CheckPoint";
	storagePtr->setExistingSaveNames({saveName});

	// Reset components to defaults to ensure they are actually changed by load
	auto playerView = registry.view<PlayerTag>();
	const Entity entity = playerView.at(0U);

	auto &jumpComponent = playerView.get<JumpComponent>(entity);
	jumpComponent.maxJumps = 0;
	auto &positionComponent = playerView.get<PositionComponent>(entity);
	positionComponent.position = glm::vec2{0.0F, 0.0F};
	auto &inventoryComponent = playerView.get<InventoryComponent>(entity);
	inventoryComponent.keys.clear();

	// 3. Perform Load
	saveSystem.loadSavedGame(registry, saveName);

	// 4. Verify
	const auto &jumpComponentAfter = playerView.get<JumpComponent>(entity);
	EXPECT_EQ(7, jumpComponentAfter.maxJumps);

	const auto &positionComponentAfter = playerView.get<PositionComponent>(entity);
	EXPECT_FLOAT_EQ(10.5F, positionComponentAfter.position.x);
	EXPECT_FLOAT_EQ(-3.0F, positionComponentAfter.position.y);

	const auto &inventoryComponentAfter = playerView.get<InventoryComponent>(entity);
	const std::vector<int> expectedKeys{1, 2, 3};
	EXPECT_EQ(expectedKeys, inventoryComponentAfter.keys);

	// Also verify the storage name was set
	EXPECT_EQ(saveName, storagePtr->getCurrentSaveName());
}

TEST_F(SaveSystemTest, LoadSavedGame_RemovesCollectedLocksAndKeys)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	json saved;
	saved["maxJumps"] = 2;
	saved["positionX"] = 1.0F;
	saved["positionY"] = 3.3F;
	saved["keys"] = std::vector<int>{9, 4, 5, 2, 1, 8};
	storagePtr->write(saved);

	// Register existence
	const std::string saveName = "TestSave";
	storagePtr->setExistingSaveNames({saveName});

	const Entity lock1 = registry.create();
	registry.registerComponent<LockComponent>(lock1, LockComponent{.lockId = 4}); // In keys

	const Entity lock2 = registry.create();
	registry.registerComponent<LockComponent>(lock2, LockComponent{.lockId = 99}); // Not in keys

	const Entity key1 = registry.create();
	registry.registerComponent<KeyComponent>(key1, KeyComponent{.id = 1}); // In keys

	const Entity key2 = registry.create();
	registry.registerComponent<KeyComponent>(key2, KeyComponent{.id = 42}); // Not in keys

	saveSystem.loadSavedGame(registry, saveName);

	// Verify Locks
	auto lockView = registry.view<LockComponent>();
	std::vector<int> remainingLocks;
	for (const Entity e : lockView) {
		remainingLocks.push_back(lockView.get<LockComponent>(e).lockId);
	}
	ASSERT_EQ(1U, remainingLocks.size());
	EXPECT_EQ(99, remainingLocks.front());

	// Verify Keys
	auto keyView = registry.view<KeyComponent>();
	std::vector<int> remainingKeys;
	for (const Entity e : keyView) {
		remainingKeys.push_back(keyView.get<KeyComponent>(e).id);
	}
	ASSERT_EQ(1U, remainingKeys.size());
	EXPECT_EQ(42, remainingKeys.front());
}

TEST_F(SaveSystemTest, LoadSavedGame_CreatesNewEmptySave_IfFileDoesNotExist)
{
	// NEW TEST: Covers the logic where a missing file initializes a new save
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	// Ensure storage says "No files exist"
	storagePtr->setExistingSaveNames({});

	// Put garbage in storage to verify it gets overwritten
	json junk;
	junk["foo"] = "bar";
	storagePtr->write(junk);

	const std::string newSaveName = "NewGameSlot";

	// Act
	saveSystem.loadSavedGame(registry, newSaveName);

	// Verify
	// 1. The name was set
	EXPECT_EQ(newSaveName, storagePtr->getCurrentSaveName());

	// 2. The data was reset to empty JSON (the cleanGameSave equivalent)
	const json stored = storagePtr->getStoredData();
	EXPECT_TRUE(stored.empty());

	// 3. The registry shouldn't have changed (empty json returns early in your code)
	auto playerView = registry.view<PlayerTag>();
	const Entity entity = playerView.at(0U);
	EXPECT_EQ(2, playerView.get<JumpComponent>(entity).maxJumps); // Should match SetUp default
}

TEST_F(SaveSystemTest, SaveGame_ThrowsIfMultiplePlayerEntitiesExist)
{
	const Entity secondPlayer = registry.create();
	registry.registerComponent<PlayerTag>(secondPlayer, PlayerTag{});

	auto storage = std::make_unique<FakeSaveStorage>();
	const SaveSystem saveSystem{std::move(storage)};

	EXPECT_THROW(saveSystem.saveGame(registry), std::runtime_error);
}

TEST_F(SaveSystemTest, LoadSavedGame_ThrowsIfMultiplePlayerEntitiesExist)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	json saved;
	saved["maxJumps"] = 2;
	saved["positionX"] = 1.0F;
	saved["positionY"] = 3.3F;
	saved["keys"] = std::vector<int>{};
	storagePtr->write(saved);

	const std::string saveName = "TestSave";
	storagePtr->setExistingSaveNames({saveName});

	const Entity secondPlayer = registry.create();
	registry.registerComponent<PlayerTag>(secondPlayer, PlayerTag{});

	EXPECT_THROW(saveSystem.loadSavedGame(registry, saveName), std::runtime_error);
}

TEST_F(SaveSystemTest, GetSaves_ReturnsStorageList)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();

	const std::vector<std::string> expectedSaves = {"Save1", "Save2", "AutoSave"};
	storagePtr->setExistingSaveNames(expectedSaves);

	const SaveSystem saveSystem{std::move(storage)};

	const auto result = saveSystem.getSaves();
	EXPECT_EQ(expectedSaves, result);
}

} // namespace
