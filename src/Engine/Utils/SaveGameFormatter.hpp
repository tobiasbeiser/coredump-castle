#pragma once

#include "Engine/FileIO/ISaveStorage.hpp"
#include <string>
#include <format>
#include <chrono>

namespace GameEngine::Utils::SaveGameFormatter {

    [[nodiscard]] inline std::string getDisplayString(const ECS::SaveMetaData& metaData)
    {
        const auto sysTime = std::chrono::clock_cast<std::chrono::system_clock>(metaData.lastSaved);
        const auto localTime = std::chrono::zoned_time{std::chrono::current_zone(), sysTime};

        return std::format("{}\n({:%F %R})", metaData.saveName, localTime);
    }

}