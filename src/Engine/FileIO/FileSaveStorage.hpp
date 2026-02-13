#pragma once

#include "ISaveStorage.hpp"

#include <filesystem>
#include <nlohmann/json.hpp>

namespace GameEngine::FileIO {

class FileSaveStorage final : public ISaveStorage {
  public:
	FileSaveStorage();
	void write(const nlohmann::json &data) override;
	[[nodiscard]] nlohmann::json read() const override;
	[[nodiscard]] std::vector<SaveMetaData> getSavesData() const override;
	void deleteSaves() override;
	void clearSaveName() override;
	[[nodiscard]] std::string getSaveName() const override;
	void setSaveName(std::string newSaveName) override;
	void deleteCurrentSave() override;

  private:
	std::filesystem::path basePath;
	std::string saveName{};

	[[nodiscard]] std::string getSaveFileName() const;
	static std::filesystem::path createSavePath();
};

} // namespace GameEngine::FileIO