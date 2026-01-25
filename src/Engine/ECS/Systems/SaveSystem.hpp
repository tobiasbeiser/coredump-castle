#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/BossComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/FileIO/ISaveStorage.hpp"

#include <nlohmann/json.hpp>
#include <set>
#include <utility>

namespace GameEngine::ECS::Systems {
using json = nlohmann::json;

class SaveSystem {
  public:
	explicit SaveSystem(std::unique_ptr<ISaveStorage> storage) : saveStorage{std::move(storage)} {}

	void saveGame(Registry &registry) const
	{
		const json saveData = buildSaveData(registry);

		if (saveStorage->getSaveName().empty())
			initNewGameSave();

		saveStorage->write(saveData);
	}

	void loadSavedGame(Registry &registry) const
	{
		const json savedGameData = saveStorage->read();
		if (savedGameData.empty()) {
			return;
		}
		reinitializePlayerGameData(registry, savedGameData);
	}

	void setCurrentSaveName(const std::string_view saveName, const bool overwrite = false) const
	{
		saveStorage->setSaveName(std::string(saveName));
		if (overwrite) {
			writeEmptyGameSave();
		}
	}

	[[nodiscard]] std::vector<SaveMetaData> getSaves() const { return saveStorage->getSavesData(); }

	void deleteAllSaves() const { saveStorage->deleteSaves(); }

  private:
	std::unique_ptr<ISaveStorage> saveStorage;

	void initNewGameSave() const
	{
		static constexpr std::string_view kPrefix = "Saved Game ";
		const auto saves = saveStorage->getSavesData();

		std::set<int> usedIds;

		for (const auto &[name, _] : saves) {
			if (!name.starts_with(kPrefix))
				continue;

			if (std::string_view numPart = std::string_view(name).substr(kPrefix.length());
			    !numPart.empty() && std::ranges::all_of(numPart, [](unsigned char c) { return std::isdigit(c); })) {
				int id = 0;
				for (const char c : numPart) {
					id = id * 10 + (c - '0');
				}
				usedIds.insert(id);
			}
		}

		int newIndex = 1;
		while (usedIds.contains(newIndex)) {
			newIndex++;
		}

		const std::string saveName = "Saved Game " + std::to_string(newIndex);

		saveStorage->setSaveName(saveName);
		writeEmptyGameSave();
	}

	void writeEmptyGameSave() const
	{
		const json emptyInit{};
		saveStorage->write(emptyInit);
	}

	[[nodiscard]] static json buildSaveData(Registry &registry)
	{
		auto playerView =
		    registry.view<PlayerTag, InventoryComponent, JumpComponent, PositionComponent, ProgressComponent>();
		if (playerView.size() > 1U) {
			throw std::runtime_error("Multiple entities with PlayerTag component");
		}
		const Entity playerEntity = playerView.at(0U);

		const auto &[keys] = playerView.get<InventoryComponent>(playerEntity);
		const auto &jumpComponent = playerView.get<JumpComponent>(playerEntity);
		const auto &[position, previousPosition] = playerView.get<PositionComponent>(playerEntity);
		const auto &progress = playerView.get<ProgressComponent>(playerEntity);

		json saveData;
		saveData["maxJumps"] = jumpComponent.maxJumps;
		saveData["positionX"] = position.x;
		saveData["positionY"] = position.y;
		saveData["keys"] = keys;
		saveData["defeatedBosses"] = progress.defeatedBosses;
		saveData["unlockedAbilities"] = progress.unlockedAbilities;

		return saveData;
	}

	static void reinitializePlayerGameData(Registry &registry, json gameData)
	{
		auto playerView =
		    registry.view<PlayerTag, InventoryComponent, JumpComponent, PositionComponent, ProgressComponent>();
		if (playerView.size() > 1U) {
			throw std::runtime_error("Multiple entities with PlayerTag component");
		}

		const Entity playerEntity = playerView.at(0U);

		auto &jumpComponent = playerView.get<JumpComponent>(playerEntity);
		auto &[position, previousPosition] = playerView.get<PositionComponent>(playerEntity);
		auto &[keys] = playerView.get<InventoryComponent>(playerEntity);
		auto &progress = playerView.get<ProgressComponent>(playerEntity);

		if (gameData.contains("maxJumps")) {
			jumpComponent.maxJumps = gameData.at("maxJumps").get<int>();
		}
		if (gameData.contains("positionX")) {
			position.x = gameData.at("positionX").get<float>();
		}
		if (gameData.contains("positionY")) {
			position.y = gameData.at("positionY").get<float>();
		}
		if (gameData.contains("keys")) {
			keys = gameData.at("keys").get<std::vector<int>>();
		}
		if (gameData.contains("defeatedBosses")) {
			progress.defeatedBosses = gameData.at("defeatedBosses").get<std::vector<int>>();
		}
		if (gameData.contains("unlockedAbilities")) {
			progress.unlockedAbilities = gameData.at("unlockedAbilities").get<std::vector<int>>();
		}

		if (progress.hasAbility(AbilityType::DoubleJump)) {
			jumpComponent.maxJumps = std::max(jumpComponent.maxJumps, 2);
		}

		for (auto lockView = registry.view<LockComponent>(); const auto lockEntity : lockView) {
			if (const auto &[lockId] = lockView.get<LockComponent>(lockEntity);
			    std::ranges::find(keys, lockId) != keys.end()) {
				registry.deleteEntity(lockEntity);
			}
		}

		for (auto keyView = registry.view<KeyComponent>(); const auto keyEntity : keyView) {
			if (const auto &[keyId] = keyView.get<KeyComponent>(keyEntity);
			    std::ranges::find(keys, keyId) != keys.end()) {
				registry.deleteEntity(keyEntity);
			}
		}

		for (auto bossView = registry.view<BossComponent>(); const auto bossEntity : bossView) {
			const auto &[bossId, unlockAbility] = bossView.get<BossComponent>(bossEntity);
			if (std::ranges::find(progress.defeatedBosses, bossId) != progress.defeatedBosses.end()) {
				registry.deleteEntity(bossEntity);
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems