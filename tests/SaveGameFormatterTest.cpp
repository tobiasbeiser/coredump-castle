#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <regex>
#include <string>

#include "Engine/Utils/SaveGameFormatter.hpp"

namespace {

using GameEngine::FileIO::SaveMetaData;
using namespace GameEngine::Utils;

TEST(SaveGameFormatterTest, GetDisplayString_FormatsStructureCorrectly)
{
	// 1. Arrange
	const std::string saveName = "Dummy Save";

	const auto fileTime = std::chrono::file_clock::now();

	const SaveMetaData metaData{saveName, fileTime};

	const std::string result = SaveGameFormatter::getDisplayString(metaData);

	const std::regex pattern(R"(^Dummy Save\n\(\d{4}-\d{2}-\d{2} \d{2}:\d{2}\)$)");

	EXPECT_TRUE(std::regex_match(result, pattern))
	    << "Output '" << result << "' did not match expected format 'Name\\n(YYYY-MM-DD HH:MM)'";
}

TEST(SaveGameFormatterTest, GetDisplayString_PreservesNameExactness)
{
	// 1. Arrange
	const std::string complexName = "Save [Auto] #1";
	const auto fileTime = std::chrono::file_clock::now();
	const SaveMetaData metaData{complexName, fileTime};

	// 2. Act
	const std::string result = SaveGameFormatter::getDisplayString(metaData);

	// 3. Assert
	EXPECT_TRUE(result.starts_with(complexName));
	EXPECT_TRUE(result.find('\n') != std::string::npos);
}

} // namespace