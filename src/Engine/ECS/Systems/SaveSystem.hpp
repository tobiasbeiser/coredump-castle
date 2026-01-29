#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/BossComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Tags/AlreadySeenTag.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
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

	void clearCurrentSaveName() const
	{
		saveStorage->clearSaveName();
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
	static constexpr std::string_view keyLabel = "keys";
	static constexpr std::string_view maxJumpsLabel = "maxJumps";
	static constexpr std::string_view positionXLabel = "positionX";
	static constexpr std::string_view positionYLabel = "positionY";
	static constexpr std::string_view seenPositionsLabel = "seenPositions";
	static constexpr std::string_view defeatedBossesLabel = "defeatedBosses";
	static constexpr std::string_view unlockedAbilitiesLabel = "unlockedAbilities";

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
		saveData[maxJumpsLabel] = jumpComponent.maxJumps;
		saveData[positionXLabel] = position.x;
		saveData[positionYLabel] = position.y;
		saveData[keyLabel] = keys;
		saveData[defeatedBossesLabel] = progress.defeatedBosses;
		saveData[unlockedAbilitiesLabel] = progress.unlockedAbilities;

		std::set<std::pair<int, int>> visitedPositions;
		for (auto view = registry.view<AlreadySeenTag, PositionComponent>().exclude<PlayerTag>();
		     const auto entity : view) {
			const auto &[tilePos, _] = view.get<PositionComponent>(entity);
			visitedPositions.emplace(tilePos.x, tilePos.y);
		}
		saveData["seenPositions"] = visitedPositions;

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

		if (gameData.contains(maxJumpsLabel)) {
			jumpComponent.maxJumps = gameData.at(maxJumpsLabel).get<int>();
		}

		if (gameData.contains(unlockedAbilitiesLabel)) {
			progress.unlockedAbilities = gameData.at(unlockedAbilitiesLabel).get<std::vector<int>>();
		}

		if (progress.hasAbility(AbilityType::DoubleJump)) {
			jumpComponent.maxJumps = std::max(jumpComponent.maxJumps, 2);
		}

		if (gameData.contains(positionXLabel) && gameData.contains(positionYLabel)) {
			position.x = gameData.at(positionXLabel).get<float>();
			position.y = gameData.at(positionYLabel).get<float>();
		}

		if (gameData.contains(keyLabel)) {
			keys = gameData.at(keyLabel).get<std::vector<int>>();
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
		}

		if (gameData.contains(defeatedBossesLabel)) {
			progress.defeatedBosses = gameData.at(defeatedBossesLabel).get<std::vector<int>>();

			for (auto bossView = registry.view<BossComponent>(); const auto bossEntity : bossView) {
				const auto &[bossId, unlockAbility] = bossView.get<BossComponent>(bossEntity);
				if (std::ranges::find(progress.defeatedBosses, bossId) != progress.defeatedBosses.end()) {
					registry.deleteEntity(bossEntity);
				}
			}
		}

		if (gameData.contains(seenPositionsLabel)) {
			for (auto view = registry.view<AlreadySeenTag>(); auto entity : view) {
				registry.remove<AlreadySeenTag>(entity);
			}

			const std::set<std::pair<int, int>> tilesSeen = gameData.at(seenPositionsLabel);
			for (auto view = registry.view<RenderComponent, PositionComponent, SpriteComponent>(); auto entity : view) {
				if (const auto [spritePos, prevPos] = view.get<PositionComponent>(entity);
				    tilesSeen.contains(std::pair(spritePos.x, spritePos.y))) {
					registry.registerComponent(entity, AlreadySeenTag{});
				}
			}
		}
	}
};
} // namespace GameEngine::ECS::Systems