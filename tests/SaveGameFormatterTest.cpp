#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <string>

#include "Engine/Utils/SaveGameFormatter.hpp"

namespace {

using GameEngine::ECS::SaveMetaData;
using namespace GameEngine::Utils;

// Helper to create a specific time point (Year, Month, Day, Hour, Minute)
// Defaults to UTC, but we convert to system_clock which is implementation defined
std::chrono::system_clock::time_point createTimePoint(const int y, const int m, const int d, const int h, const int min) {
    using namespace std::chrono;

    // Construct a specific date/time (e.g., 2023-10-25 14:30:00)
    // We use days-based arithmetic for safety across C++ versions
    const year_month_day yearMonthDay{year{y}, month{static_cast<unsigned>(m)}, day{static_cast<unsigned>(d)}};
    const auto dateTp = sys_days{yearMonthDay};

    return dateTp + hours{h} + minutes{min};
}

TEST(SaveGameFormatterTest, GetDisplayString_FormatsNameAndDateCorrectly)
{
    // 1. Arrange
    const std::string saveName = "Level 1 - Boss Room";

    // Create a fixed time: 2023-05-20 15:45 UTC (System Time)
    const auto fixedSysTime = createTimePoint(2023, 5, 20, 15, 45);

    // Convert System Time -> File Time (Input format)
    // Note: clock_cast is C++20/23 feature required here
    const auto fileTime = std::chrono::clock_cast<std::filesystem::file_time_type::clock>(fixedSysTime);

    const SaveMetaData metaData{saveName, fileTime};

    // 2. Act
    const std::string result = SaveGameFormatter::getDisplayString(metaData);

    // 3. Assert
    // To make this robust regardless of the test runner's Time Zone,
    // we construct the expected string using the known logic.
    // This verifies the *Format String* logic ("{}\n({:%F %R})") is applied correctly.
    const auto localTime = std::chrono::zoned_time{std::chrono::current_zone(), fixedSysTime};
    const std::string expected = std::format("{}\n({:%F %R})", saveName, localTime);

    EXPECT_EQ(expected, result);
}

TEST(SaveGameFormatterTest, GetDisplayString_HandlesEmptySaveName)
{
    // 1. Arrange
    const std::string emptyName = "";
    const auto nowSys = std::chrono::system_clock::now();
    const auto nowFile = std::chrono::clock_cast<std::filesystem::file_time_type::clock>(nowSys);

    const SaveMetaData metaData{emptyName, nowFile};

    // 2. Act
    const std::string result = SaveGameFormatter::getDisplayString(metaData);

    // 3. Assert
    const auto localTime = std::chrono::zoned_time{std::chrono::current_zone(), nowSys};
    // Expect: "\n(YYYY-MM-DD HH:MM)"
    const std::string expected = std::format("{}\n({:%F %R})", emptyName, localTime);

    EXPECT_EQ(expected, result);
}

TEST(SaveGameFormatterTest, GetDisplayString_HandlesSpecialCharacters)
{
    // 1. Arrange
    // Ensure format doesn't break on brackets or unicode (if std::format supports it)
    const std::string weirdName = "Save [Auto] #1";
    const auto fixedSysTime = createTimePoint(2024, 1, 1, 9, 0); // 9:00 AM
    const auto fileTime = std::chrono::clock_cast<std::filesystem::file_time_type::clock>(fixedSysTime);

    const SaveMetaData metaData{weirdName, fileTime};

    // 2. Act
    const std::string result = SaveGameFormatter::getDisplayString(metaData);

    // 3. Assert
    EXPECT_TRUE(result.starts_with("Save [Auto] #1"));
    EXPECT_TRUE(result.find('\n') != std::string::npos);
}

} // namespace