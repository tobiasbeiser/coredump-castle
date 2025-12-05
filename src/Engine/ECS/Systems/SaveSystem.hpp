#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/JumpComponent.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/FileIO/ISaveStorage.hpp"

#include <nlohmann/json.hpp>
#include <utility>

namespace GameEngine::ECS::Systems {
using json = nlohmann::json;

class SaveSystem {
  public:
	explicit SaveSystem(std::unique_ptr<ISaveStorage> storage) : saveStorage{std::move(storage)} {}

	void saveGame(Registry &registry) const
	{
		const json saveData = buildSaveData(registry);

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

	[[nodiscard]] std::vector<std::string> getSaves() const
	{
		return saveStorage->getSavesNames();
	}

	void setSaveName(const std::string &saveName) const
	{
		saveStorage->setSaveName(saveName);
	}

	void initNewSave() const
	{
		const auto saveFilesCount = saveStorage->getSavesNames().size();
		cleanGameSave("SavedGame" + std::to_string(saveFilesCount));
	}

	void cleanGameSave(std::string saveName) const
	{
		saveStorage->setSaveName(std::move(saveName));
		const json emptyInit {};
		saveStorage->write(emptyInit);
	}


  private:
	std::unique_ptr<ISaveStorage> saveStorage;

	[[nodiscard]] static json buildSaveData(Registry &registry)
	{
		auto playerView = registry.view<PlayerTag>();
		if (playerView.size() > 1U) {
			throw std::runtime_error("Multiple entities with PlayerTag component");
		}
		const Entity playerEntity = playerView.at(0U);

		const auto &[keys] = playerView.get<InventoryComponent>(playerEntity);
		const auto &jumpComponent = playerView.get<JumpComponent>(playerEntity);
		const auto &[position, previousPosition] = playerView.get<PositionComponent>(playerEntity);

		json saveData;
		saveData["maxJumps"] = jumpComponent.maxJumps;
		saveData["positionX"] = position.x;
		saveData["positionY"] = position.y;
		saveData["keys"] = keys;

		return saveData;
	}

	static void reinitializePlayerGameData(Registry &registry, json gameData)
	{
		auto playerView = registry.view<PlayerTag>();
		if (playerView.size() > 1U) {
			throw std::runtime_error("Multiple entities with PlayerTag component");
		}

		const Entity playerEntity = playerView.at(0U);

		auto &jumpComponent = playerView.get<JumpComponent>(playerEntity);
		auto &[position, previousPosition] = playerView.get<PositionComponent>(playerEntity);
		auto &[keys] = playerView.get<InventoryComponent>(playerEntity);

		jumpComponent.maxJumps = gameData.at("maxJumps").get<int>();
		position.x = gameData.at("positionX").get<float>();
		position.y = gameData.at("positionY").get<float>();
		keys = gameData.at("keys").get<std::vector<int>>();

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
};
} // namespace GameEngine::ECS::Systems