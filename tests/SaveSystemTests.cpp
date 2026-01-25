
#include <gtest/gtest.h>
#include <vector>
#include <chrono>
#include <filesystem>
#include <algorithm>

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/BossComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/SaveSystem.hpp"

namespace {

using GameEngine::ECS::Entity;
using GameEngine::ECS::ISaveStorage;
using GameEngine::ECS::Registry;
using GameEngine::ECS::SaveMetaData;
using GameEngine::ECS::Systems::SaveSystem;
using json = nlohmann::json;
using namespace std::chrono_literals;

class FakeSaveStorage final : public ISaveStorage {
  public:
	void write(const json &data) override { store = data; }

	[[nodiscard]] json read() const override { return store; }

    [[nodiscard]] std::vector<SaveMetaData> getSavesData() const override {
        return existingSaves;
    }

    void deleteSaves() override {
        existingSaves.clear();
        store.clear();
        deletionCalled = true;
    }

    [[nodiscard]] std::string getSaveName() const override { return currentSaveName; }
    void setSaveName(std::string saveName) override { currentSaveName = std::move(saveName); }

    [[nodiscard]] const json &getStoredData() const { return store; }

    void setExistingSaves(std::vector<SaveMetaData> saves) { existingSaves = std::move(saves); }

    [[nodiscard]] bool wasDeleteCalled() const { return deletionCalled; }

  private:
    json store{};
    std::string currentSaveName;
    std::vector<SaveMetaData> existingSaves;
    bool deletionCalled = false;
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

		JumpComponent jumpComponent{.maxJumps = 2, .coyoteTimer = Timer{0.25s}};
		registry.registerComponent<JumpComponent>(playerEntity, jumpComponent);

		PositionComponent positionComponent{};
		positionComponent.position = glm::vec2{1.0F, 3.3F};
		registry.registerComponent<PositionComponent>(playerEntity, positionComponent);

		InventoryComponent inventoryComponent{};
		inventoryComponent.keys = std::vector{9, 4, 5, 2, 1, 8};
		registry.registerComponent<InventoryComponent>(playerEntity, inventoryComponent);

		registry.registerComponent<ProgressComponent>(playerEntity, ProgressComponent{});
	}
};

TEST_F(SaveSystemTest, SaveGame_ProducesExpectedJson)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	storagePtr->setSaveName("QuickSave");
	saveSystem.saveGame(registry);

	const json data = storagePtr->read();

	EXPECT_EQ(2, data.at("maxJumps").get<int>());
	EXPECT_FLOAT_EQ(1.0F, data.at("positionX").get<float>());
	EXPECT_FLOAT_EQ(3.3F, data.at("positionY").get<float>());

	const auto keysFromFile = data.at("keys").get<std::vector<int>>();
	const std::vector<int> expectedKeys{9, 4, 5, 2, 1, 8};
	EXPECT_EQ(expectedKeys, keysFromFile);

	const auto defeatedBosses = data.at("defeatedBosses").get<std::vector<int>>();
	const auto unlockedAbilities = data.at("unlockedAbilities").get<std::vector<int>>();
	EXPECT_TRUE(defeatedBosses.empty());
	EXPECT_TRUE(unlockedAbilities.empty());
}

TEST_F(SaveSystemTest, SaveGame_IncludesProgressData)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	auto playerView = registry.view<PlayerTag, ProgressComponent>();
	const Entity entity = playerView.at(0U);
	auto &progress = playerView.get<ProgressComponent>(entity);
	progress.defeatedBosses = {1, 5};
	progress.unlockedAbilities = {static_cast<int>(AbilityType::DoubleJump)};

	storagePtr->setSaveName("QuickSave");
	saveSystem.saveGame(registry);

	const json data = storagePtr->read();
	EXPECT_EQ(progress.defeatedBosses, data.at("defeatedBosses").get<std::vector<int>>());
	EXPECT_EQ(progress.unlockedAbilities, data.at("unlockedAbilities").get<std::vector<int>>());
}

TEST_F(SaveSystemTest, LoadSavedGame_RestoresPlayerStateFromJson)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	// Setup the fake data
	json saved;
	saved["maxJumps"] = 7;
	saved["positionX"] = 10.5F;
	saved["positionY"] = -3.0F;
	saved["keys"] = std::vector{1, 2, 3};
	storagePtr->write(saved);

	// Reset components to defaults to ensure they are actually changed by load
	auto playerView = registry.view<PlayerTag, JumpComponent, PositionComponent, InventoryComponent>();
	const Entity entity = playerView.at(0U);

	auto &jumpComponent = playerView.get<JumpComponent>(entity);
	jumpComponent.maxJumps = 0;
	auto &positionComponent = playerView.get<PositionComponent>(entity);
	positionComponent.position = glm::vec2{0.0F, 0.0F};
	auto &inventoryComponent = playerView.get<InventoryComponent>(entity);
	inventoryComponent.keys.clear();

	// Perform Load
	saveSystem.loadSavedGame(registry);

	// Verify
	const auto &jumpComponentAfter = playerView.get<JumpComponent>(entity);
	EXPECT_EQ(7, jumpComponentAfter.maxJumps);

	const auto &positionComponentAfter = playerView.get<PositionComponent>(entity);
	EXPECT_FLOAT_EQ(10.5F, positionComponentAfter.position.x);
	EXPECT_FLOAT_EQ(-3.0F, positionComponentAfter.position.y);

	const auto &inventoryComponentAfter = playerView.get<InventoryComponent>(entity);
	const std::vector expectedKeys{1, 2, 3};
	EXPECT_EQ(expectedKeys, inventoryComponentAfter.keys);
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

	const Entity lock1 = registry.create();
	registry.registerComponent<LockComponent>(lock1, LockComponent{.lockId = 4}); // In keys

	const Entity lock2 = registry.create();
	registry.registerComponent<LockComponent>(lock2, LockComponent{.lockId = 99}); // Not in keys

	const Entity key1 = registry.create();
	registry.registerComponent<KeyComponent>(key1, KeyComponent{.id = 1}); // In keys

	const Entity key2 = registry.create();
	registry.registerComponent<KeyComponent>(key2, KeyComponent{.id = 42}); // Not in keys

	saveSystem.loadSavedGame(registry);

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

TEST_F(SaveSystemTest, SaveGame_CreatesNewEmptySave_IfFileDoesNotExist)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};
    storagePtr->setExistingSaves({});

	// Put garbage in storage to verify it gets overwritten
	const std::string junkKey = "foo";
	json junk;
	junk[junkKey] = "bar";
	storagePtr->write(junk);

	// Act
	saveSystem.saveGame(registry);

	// Verify
	// 1. The name was set
	EXPECT_TRUE(storagePtr->getSaveName().starts_with("Saved Game"));

	// 2. The data was reset
	const json stored = storagePtr->getStoredData();
	EXPECT_FALSE(stored.contains(junkKey));
}

TEST_F(SaveSystemTest, SaveGame_ThrowsIfMultiplePlayerEntitiesExist)
{
	const Entity secondPlayer = registry.create();
	registry.registerComponent<PlayerTag>(secondPlayer, PlayerTag{});
	registry.registerComponent<InventoryComponent>(secondPlayer, {});
	registry.registerComponent<JumpComponent>(secondPlayer, {});
	registry.registerComponent<PositionComponent>(secondPlayer, {});
	registry.registerComponent<ProgressComponent>(secondPlayer, {});

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

	const Entity secondPlayer = registry.create();
	registry.registerComponent<PlayerTag>(secondPlayer, PlayerTag{});
	registry.registerComponent<InventoryComponent>(secondPlayer, {});
	registry.registerComponent<JumpComponent>(secondPlayer, {});
	registry.registerComponent<PositionComponent>(secondPlayer, {});
	registry.registerComponent<ProgressComponent>(secondPlayer, {});

	EXPECT_THROW(saveSystem.loadSavedGame(registry), std::runtime_error);
}

TEST_F(SaveSystemTest, GetSaves_ReturnsStorageListWithMetadata)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();

    // Create a fixed time point for testing
    const auto now = std::filesystem::file_time_type::clock::now();

    const std::vector<SaveMetaData> expectedSaves = {
        {"Save1", now},
        {"Save2", now}
    };
    storagePtr->setExistingSaves(expectedSaves);

	const SaveSystem saveSystem{std::move(storage)};

	const auto result = saveSystem.getSaves();

    ASSERT_EQ(expectedSaves.size(), result.size());
    EXPECT_EQ(expectedSaves[0].saveName, result[0].saveName);
    EXPECT_EQ(expectedSaves[0].lastSaved, result[0].lastSaved);
    EXPECT_EQ(expectedSaves[1].saveName, result[1].saveName);
}

TEST_F(SaveSystemTest, RoundTrip_SaveThenLoad_PreservesState)
{
    auto storage = std::make_unique<FakeSaveStorage>();
    auto *storagePtr = storage.get();
    const SaveSystem saveSystem{std::move(storage)};

	// 1. Modify initial state to be distinct from defaults
	auto playerView = registry.view<PlayerTag, JumpComponent, PositionComponent, ProgressComponent>();
	const Entity entity = playerView.at(0U);

	auto &jump = playerView.get<JumpComponent>(entity);
	jump.maxJumps = 99;

	auto &pos = playerView.get<PositionComponent>(entity);
	pos.position = glm::vec2{500.0F, 500.0F};

    // 2. Save
    const std::string saveName = "RoundTripSave";
    storagePtr->setSaveName(saveName);
    saveSystem.saveGame(registry);

    // Simulate file existing now (requires SaveMetaData construction)
    storagePtr->setExistingSaves({
        SaveMetaData{saveName, std::filesystem::file_time_type::clock::now()}
    });

    // 3. Corrupt the registry
    jump.maxJumps = 0;
    pos.position = glm::vec2{0.0F, 0.0F};

    // 4. Load back
    saveSystem.loadSavedGame(registry);

    // 5. Verify restoration
    EXPECT_EQ(99, jump.maxJumps);
    EXPECT_FLOAT_EQ(500.0F, pos.position.x);
    EXPECT_FLOAT_EQ(500.0F, pos.position.y);
}

TEST_F(SaveSystemTest, LoadSavedGame_Throws_WhenSaveDataIsCorrupt)
{
    auto storage = std::make_unique<FakeSaveStorage>();
    auto *storagePtr = storage.get();
    const SaveSystem saveSystem{std::move(storage)};

	// Create incomplete JSON (missing 'maxJumps')
	json corruptData;
	corruptData["positionX"] = 10.0F;
	corruptData["positionY"] = 10.0F;
	corruptData["keys"] = std::vector<int>{};

    storagePtr->write(corruptData);

    storagePtr->setExistingSaves({
        SaveMetaData{"CorruptFile", std::filesystem::file_time_type::clock::now()}
    });

	EXPECT_NO_THROW(saveSystem.loadSavedGame(registry));
}

TEST_F(SaveSystemTest, LoadSavedGame_RemovesDefeatedBossEntities)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	json saved;
	saved["maxJumps"] = 1;
	saved["positionX"] = 0.0F;
	saved["positionY"] = 0.0F;
	saved["keys"] = std::vector<int>{};
	saved["defeatedBosses"] = std::vector<int>{7};
	saved["unlockedAbilities"] = std::vector<int>{static_cast<int>(AbilityType::DoubleJump)};
	storagePtr->write(saved);

	const Entity bossDefeated = registry.create();
	registry.registerComponent<BossComponent>(bossDefeated, BossComponent{.bossId = 7,
	                                                                    .unlockAbility = AbilityType::DoubleJump});

	const Entity bossAlive = registry.create();
	registry.registerComponent<BossComponent>(bossAlive, BossComponent{.bossId = 9,
	                                                                  .unlockAbility = AbilityType::None});

	saveSystem.loadSavedGame(registry);

	auto bossView = registry.view<BossComponent>();
	std::vector<int> remainingBosses;
	for (const Entity e : bossView) {
		remainingBosses.push_back(bossView.get<BossComponent>(e).bossId);
	}
	ASSERT_EQ(1U, remainingBosses.size());
	EXPECT_EQ(9, remainingBosses.front());
}

TEST_F(SaveSystemTest, LoadSavedGame_KeepsBossIfNotDefeated)
{
	auto storage = std::make_unique<FakeSaveStorage>();
	auto *storagePtr = storage.get();
	const SaveSystem saveSystem{std::move(storage)};

	json saved;
	saved["maxJumps"] = 1;
	saved["positionX"] = 0.0F;
	saved["positionY"] = 0.0F;
	saved["keys"] = std::vector<int>{};
	saved["defeatedBosses"] = std::vector<int>{};
	saved["unlockedAbilities"] = std::vector<int>{};
	storagePtr->write(saved);

	const Entity bossAlive = registry.create();
	registry.registerComponent<BossComponent>(bossAlive, BossComponent{.bossId = 3,
	                                                                  .unlockAbility = AbilityType::None});

	saveSystem.loadSavedGame(registry);

	auto bossView = registry.view<BossComponent>();
	std::vector<int> remainingBosses;
	for (const Entity e : bossView) {
		remainingBosses.push_back(bossView.get<BossComponent>(e).bossId);
	}
	ASSERT_EQ(1U, remainingBosses.size());
	EXPECT_EQ(3, remainingBosses.front());
}

TEST_F(SaveSystemTest, DeleteAllSaves_DelegatesToStorage)
{
    auto storage = std::make_unique<FakeSaveStorage>();
    auto *storagePtr = storage.get();

    // Populate with dummy data
    storagePtr->setExistingSaves({
        SaveMetaData{"Save1", std::filesystem::file_time_type::clock::now()}
    });

    const SaveSystem saveSystem{std::move(storage)};

    // Act
    saveSystem.deleteAllSaves();

    // Verify
    EXPECT_TRUE(storagePtr->wasDeleteCalled());
    EXPECT_TRUE(storagePtr->getSavesData().empty());
}

} // namespace
