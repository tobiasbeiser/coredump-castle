#pragma once
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace GameEngine::ECS::Systems {

class SaveSystem {
public:

	static void saveGame(Registry &registry)
	{
		const json saveData = buildSaveData(registry);
		const auto prefPath = SDL_GetPrefPath("AdvancedCpp", "CoredumpCastle");

		if (!prefPath) {
			throw std::runtime_error(SDL_GetError());
		}

		std::ofstream saveFile(std::filesystem::path(prefPath) /  "SaveGame.json", std::ios::trunc);
		saveFile << saveData.dump(4);
		saveFile.close();
		SDL_free(prefPath);
	}

	static void loadSavedGame(Registry &registry)
	{
		const auto prefPath = SDL_GetPrefPath("AdvancedCpp", "CoredumpCastle");

		if (!prefPath) {
			throw std::runtime_error(SDL_GetError());
		}

		std::ifstream savedGameStream(std::filesystem::path(prefPath) /  "SaveGame.json");
		const json savedGameData = json::parse(savedGameStream);
		savedGameStream.close();

		reinitializePlayerGameData(registry, savedGameData);
		SDL_free(prefPath);
	}

private:
	static json buildSaveData(Registry &registry)
	{
		auto playerView = registry.view<PlayerTag>();
		if (playerView.size() > 1) {
			throw std::runtime_error("Multiple entities with PlayerTag component");
		}
		const Entity playerEntity = playerView.at(0);
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
		if (playerView.size() > 1) {
			throw std::runtime_error("Multiple entities with PlayerTag component");
		}

		const Entity playerEntity = playerView.at(0);
		auto &jumpComponent = playerView.get<JumpComponent>(playerEntity);
		auto &[position, previousPosition] = playerView.get<PositionComponent>(playerEntity);
		auto &[keys] = playerView.get<InventoryComponent>(playerEntity);

		jumpComponent.maxJumps = gameData.at("maxJumps");
		position.x = gameData.at("positionX");
		position.y = gameData.at("positionY");
		keys = static_cast<std::vector<int>>(gameData.at("keys"));

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