#pragma once

#include "ISaveStorage.hpp"

#include <SDL3/SDL_filesystem.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace GameEngine::ECS {

class FileSaveStorage final : public ISaveStorage {
  public:
	static constexpr std::string_view fileExtension = ".json";

	void write(const nlohmann::json &data) override
	{
		const auto path = getSavePath() / getSaveFileName();

		std::ofstream saveFile(path, std::ios::trunc);
		if (!saveFile.is_open()) {
			throw std::runtime_error("Failed to open save file for writing");
		}

		saveFile << data.dump(4);
		if (!saveFile) {
			throw std::runtime_error("Failed to write save file");
		}
	}

	[[nodiscard]] nlohmann::json read() const override
	{
		const auto path = getSavePath() / getSaveFileName();
		if (!std::filesystem::exists(path)) {
			throw std::runtime_error("Save file path " + path.string() + " does not exist");
		}

		std::ifstream savedGameStream(path);

		if (!savedGameStream.is_open()) {
			throw std::runtime_error("Failed to open save file for reading");
		}

		return nlohmann::json::parse(savedGameStream);
	}

	[[nodiscard]] std::vector<std::string> getSavesNames() const override
	{
		std::vector<std::string> saveFileNames;

		for (auto dirIterator = std::filesystem::directory_iterator(getSavePath());
		     const auto &dirEntry : dirIterator) {

			if (dirEntry.is_regular_file() && dirEntry.path().extension() == fileExtension) {
				saveFileNames.emplace_back(dirEntry.path().stem().string());
			}
		}

		return saveFileNames;
	}

	void setSaveName(std::string newSaveName) override { saveName = std::move(saveName); }

  private:
	std::string saveName{};

	[[nodiscard]] std::string getSaveFileName() const
	{
		if (saveName.empty()) {
			return std::string{};
		}
		return saveName + std::string(fileExtension);
	}

	static std::filesystem::path getSavePath()
	{
		using SdlPathPtr = std::unique_ptr<char, decltype(&SDL_free)>;
		const SdlPathPtr prefPath{SDL_GetPrefPath("AdvancedCpp", "CoredumpCastle"), &SDL_free};

		if (prefPath == nullptr) {
			throw std::runtime_error(SDL_GetError());
		}

		return std::filesystem::path{prefPath.get()};
	}
};

} // namespace GameEngine::ECS