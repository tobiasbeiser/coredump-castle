#pragma once
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/ECS/Components/Gameplay/KeyComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InteractionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InventoryComponent.hpp"
#include "Engine/ECS/Components/Gameplay/LockComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

constexpr std::string directoryName = "savefiles";
namespace GameEngine::ECS::Systems {
using json = nlohmann::json;

class SaveSystem {
public:

	static void saveGame(Registry &registry)
	{

		const json saveData = buildSaveData(registry);
		const auto fullPath = std::filesystem::current_path() / directoryName;


		if (!std::filesystem::exists(fullPath)) {
			std::filesystem::create_directories(fullPath);
		}
		std::ofstream saveFile(fullPath / "SaveGame.json", std::ios::trunc);
		saveFile << saveData.dump(4);
		saveFile.close();
	}

	static void removeSavedGame()
	{
		namespace fs = std::filesystem;
		if (const auto fullPath = fs::current_path() / directoryName; fs::exists(fullPath)) {
			fs::remove_all(fullPath);
		}
	}


	static void loadSavedGame(Registry &registry)
	{


		// for (auto interactableObjects = registry.view<PositionComponent, InteractionComponent>();
		// 	 auto &interactableObject : interactableObjects) {
		// 	if (registry.has<KeyComponent>(interactableObject)) {
		// 		const auto [id] = registry.get<KeyComponent>(interactableObject);
		// 		inventory.emplace_back(id);
		// 		registry.deleteEntity(interactableObject);
		//
		// 		for (auto lockView = registry.view<LockComponent>(); const auto lockEntity : lockView) {
		// 			if (const auto &[lockId] = lockView.get<LockComponent>(lockEntity); lockId == id) {
		// 				registry.deleteEntity(lockEntity);
		// 			}
		// 		}
		// 	}
		// }

	}

private:
	static json buildSaveData(Registry &registry)
	{
		auto playerView = registry.view<PlayerTag>();
		if (playerView.size() > 1) {

		}
		const Entity playerEntity = playerView.at(0);
		const auto &[keys] = playerView.get<InventoryComponent>(playerEntity);
		const auto &jumpComponent = playerView.get<JumpComponent>(playerEntity);
		const auto &[position, previousPosition] = playerView.get<PositionComponent>(playerEntity);
		json saveData = {
			{"maxJumps", jumpComponent.maxJumps},
			{"positionX", position.x},
			{"positionY", position.y},
			{"keys", keys},
		};
		return saveData;
	}
};
} // namespace GameEngine::ECS::Systems