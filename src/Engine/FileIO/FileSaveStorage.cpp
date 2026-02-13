#include "Engine/FileIO/FileSaveStorage.hpp"
#include <SDL3/SDL_filesystem.h>
#include <fstream>

namespace GameEngine::FileIO {

static constexpr std::string_view fileExtension = ".json";

FileSaveStorage::FileSaveStorage() : basePath{createSavePath()}
{
	if (!std::filesystem::exists(basePath)) {
		std::filesystem::create_directories(basePath);
	}
}

void FileSaveStorage::write(const nlohmann::json &data)
{
	const auto finalPath = basePath / getSaveFileName();
	auto tempPath = finalPath;
	tempPath += ".tmp";

	std::ofstream saveFile(tempPath, std::ios::trunc);
	if (!saveFile.is_open()) {
		throw std::runtime_error("Failed to open temp save file: " + tempPath.string());
	}

	saveFile << std::setw(4) << data;

	saveFile.flush();

	if (!saveFile) {
		saveFile.close();
		std::filesystem::remove(tempPath); // Clean up the garbage file
		throw std::runtime_error("Failed to write save data (Disk full or permission error)");
	}

	// Explicitly close before renaming to release the file lock
	saveFile.close();

	std::error_code ec;
	std::filesystem::rename(tempPath, finalPath, ec);

	if (ec) {
		std::filesystem::remove(tempPath);
		throw std::runtime_error("Failed to commit save file: " + ec.message());
	}
}

nlohmann::json FileSaveStorage::read() const
{
	if (getSaveFileName().empty()) {
		return nlohmann::json::object(); // return empty json to skip reinitialization
	}

	const auto path = basePath / getSaveFileName();
	if (!std::filesystem::exists(path)) {
		throw std::runtime_error("Save file path " + path.string() + " does not exist");
	}

	std::ifstream savedGameStream(path);

	if (!savedGameStream.is_open()) {
		throw std::runtime_error("Failed to open save file for reading");
	}

	return nlohmann::json::parse(savedGameStream);
}

std::vector<SaveMetaData> FileSaveStorage::getSavesData() const
{
	std::vector<SaveMetaData> savesMetaData;
	// Use cached basePath
	if (!std::filesystem::exists(basePath))
		return savesMetaData;

	for (const auto &dirEntry : std::filesystem::directory_iterator(basePath)) {

		if (dirEntry.is_regular_file() && dirEntry.path().extension() == fileExtension) {
			savesMetaData.emplace_back(dirEntry.path().stem().string(), dirEntry.last_write_time());
		}
	}
	std::ranges::sort(savesMetaData,
	                  [](const SaveMetaData &a, const SaveMetaData &b) { return a.lastSaved > b.lastSaved; });

	return savesMetaData;
}

void FileSaveStorage::deleteSaves()
{
	for (const auto &entry : std::filesystem::directory_iterator(basePath))
		std::filesystem::remove_all(entry.path());
}

void FileSaveStorage::clearSaveName()
{
	saveName = "";
}

std::string FileSaveStorage::getSaveName() const
{
	return saveName;
}

void FileSaveStorage::setSaveName(const std::string newSaveName)
{
	saveName = newSaveName;
}

void FileSaveStorage::deleteCurrentSave()
{
	std::filesystem::remove_all(basePath / getSaveFileName());
	clearSaveName();
}

std::string FileSaveStorage::getSaveFileName() const
{
	if (saveName.empty())
		return {};
	return saveName + std::string(fileExtension);
}

std::filesystem::path FileSaveStorage::createSavePath()
{
	using SdlPathPtr = std::unique_ptr<char, decltype(&SDL_free)>;
	const SdlPathPtr prefPath{SDL_GetPrefPath("AdvancedCpp", "CoredumpCastle"), &SDL_free};

	if (prefPath == nullptr) {
		throw std::runtime_error(SDL_GetError());
	}

	return std::filesystem::path{prefPath.get()};
}
} // namespace GameEngine::FileIO
