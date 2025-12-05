#pragma once

#include "ISaveStorage.hpp"

#include <chrono>
#include <SDL3/SDL_filesystem.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <utility>

namespace GameEngine::ECS {


class FileSaveStorage final : public ISaveStorage {
  public:

	void write(const nlohmann::json &data) override
	{
		const auto path = getSavePath() / fileName;

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
		const auto path = getSavePath() / fileName;
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
		std::vector<std::string>saveFileNames;

		for (auto dirIterator = std::filesystem::directory_iterator(getSavePath());
			const auto &dirEntry : dirIterator) {
			const auto baseString = dirEntry.path().filename();

			saveFileNames.emplace_back(baseString);
			}

		return saveFileNames;
	}

	void setSaveName(std::string saveName) override
	{
		if (!saveName.ends_with(".json"))
			saveName += ".json";
		fileName = std::move(saveName);
	}

  private:
	std::string fileName {};

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