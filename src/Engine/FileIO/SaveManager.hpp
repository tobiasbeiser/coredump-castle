#pragma once
#include "Engine/ECS/Registry.hpp"
#include "ISaveStorage.hpp"

#include <nlohmann/json.hpp>

namespace GameEngine::FileIO {
using json = nlohmann::json;

class SaveManager {
  public:
	explicit SaveManager(std::unique_ptr<ISaveStorage> storage);

	bool isSaveNameSet() const;
	void saveGame(ECS::Registry &registry) const;
	void loadSavedGame(ECS::Registry &registry) const;
	void clearCurrentSaveName() const;
	void setCurrentSaveName(std::string_view saveName, bool overwrite = false) const;
	[[nodiscard]] std::vector<SaveMetaData> getSaves() const;
	void deleteAllSaves() const;
	void deleteCurrentSave() const;

  private:
	void initNewGameSave() const;
	void writeEmptyGameSave() const;
	[[nodiscard]] static json buildSaveData(ECS::Registry &registry);
	static void reinitializePlayerGameData(ECS::Registry &registry, json gameData);
	std::unique_ptr<ISaveStorage> saveStorage;
};
} // namespace GameEngine::FileIO