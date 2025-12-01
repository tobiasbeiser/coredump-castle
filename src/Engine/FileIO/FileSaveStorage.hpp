#pragma once

#include "ISaveStorage.hpp"

#include <chrono>
#include <SDL3/SDL_filesystem.h>
#include <filesystem>
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>

namespace GameEngine::ECS {

namespace Detail {

[[nodiscard]] inline std::filesystem::path getSavePath()
{
	using SdlPathPtr = std::unique_ptr<char, decltype(&SDL_free)>;
	const SdlPathPtr prefPath{SDL_GetPrefPath("AdvancedCpp", "CoredumpCastle"), &SDL_free};

	if (prefPath == nullptr) {
		throw std::runtime_error(SDL_GetError());
	}

	return std::filesystem::path{prefPath.get()};
}

[[nodiscard]] inline std::vector<std::string> getSaveFileStrings()
{
	constexpr auto maxFileAmount = 3;
	std::vector<std::string>saveFileNames;

	using namespace std::chrono;

	for (auto dirIterator = std::filesystem::directory_iterator(getSavePath());
		const auto &dirEntry : dirIterator) {
		const auto baseString = dirEntry.path().filename().stem().string();

		const auto lastTime = std::filesystem::last_write_time(dirEntry);
		const auto localTime = zoned_time{current_zone(), clock_cast<system_clock>(lastTime)};

		auto timeAppendix = std::format(", Last Saved: {0:%y}-{0:%m}-{0:%d} {0:%R}", localTime);

		saveFileNames.emplace_back(baseString + timeAppendix);
	}
	auto counter = saveFileNames.size();

	while (counter < maxFileAmount) {
		counter++;
		saveFileNames.emplace_back("SaveSlot" + std::to_string(counter));
	}

	return saveFileNames;
}

};

class FileSaveStorage final : public ISaveStorage {
  public:

	explicit FileSaveStorage(std::string fileName) : fileName(std::move(fileName)) {};

	void write(const nlohmann::json &data) override
	{
		const auto path = Detail::getSavePath() / fileName;

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
		const auto path = Detail::getSavePath() / fileName;
		if (!std::filesystem::exists(path)) {
			throw std::runtime_error("Save file path " + path.string() + " does not exist");
		}

		std::ifstream savedGameStream(path);

		if (!savedGameStream.is_open()) {
			throw std::runtime_error("Failed to open save file for reading");
		}

		return nlohmann::json::parse(savedGameStream);
	}

  private:
	std::string fileName;
};

} // namespace GameEngine::ECS