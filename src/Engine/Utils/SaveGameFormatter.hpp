#pragma once

#include "Engine/FileIO/ISaveStorage.hpp"
#include "TimeUtils.hpp"
#include <chrono>
#include <string>

namespace GameEngine::Utils::SaveGameFormatter {
[[nodiscard]] inline std::string getDisplayString(const FileIO::SaveMetaData &metaData)
{
	const auto sysTime = toSystemClock(metaData.lastSaved);
	const std::tm localTm = toLocalTime(sysTime);

	std::stringstream ss;
	ss << metaData.saveName << "\n(" << std::put_time(&localTm, "%F %R") << ")";

	return ss.str();
}
} // namespace GameEngine::Utils::SaveGameFormatter