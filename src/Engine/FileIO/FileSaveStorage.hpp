#pragma once

#include "ISaveStorage.hpp"

#include <SDL3/SDL_filesystem.h>
#include <filesystem>
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

	return std::filesystem::path{prefPath.get()} / "SaveGame.json";
}

} // namespace Detail

class FileSaveStorage final : public ISaveStorage {
  public:
	void write(const json &data) override
	{
		const auto path = Detail::getSavePath();

		std::ofstream saveFile(path, std::ios::trunc);
		if (!saveFile.is_open()) {
			throw std::runtime_error("Failed to open save file for writing");
		}

		saveFile << data.dump(4);
		if (!saveFile) {
			throw std::runtime_error("Failed to write save file");
		}
	}

	[[nodiscard]] json read() const override
	{
		const auto path = Detail::getSavePath();

		std::ifstream savedGameStream(path);
		if (!savedGameStream.is_open()) {
			throw std::runtime_error("Failed to open save file for reading");
		}

		return json::parse(savedGameStream);
	}
};

} // namespace GameEngine::ECS