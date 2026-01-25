#include "Engine/Utils/TimeUtils.hpp"
#include <gtest/gtest.h>

// Use a known stable clock for testing
using namespace GameEngine::Utils;

TEST(TimeUtilsTest, ToSystemClock_PreservesSystemClock)
{
	const auto now = std::chrono::system_clock::now();
	const auto result = toSystemClock(now);
	EXPECT_EQ(now, result);
}

TEST(TimeUtilsTest, ToSystemClock_ConvertsSteadyClockCorrectly)
{

	const auto steadyNow = std::chrono::steady_clock::now();
	const auto sysNowPre = std::chrono::system_clock::now();

	const auto converted = toSystemClock(steadyNow);

	const auto sysNowPost = std::chrono::system_clock::now();

	EXPECT_GE(converted, sysNowPre - std::chrono::milliseconds(10));
	EXPECT_LE(converted, sysNowPost + std::chrono::milliseconds(10));
}

TEST(TimeUtilsTest, ToLocalTime_ReturnsValidStruct)
{
	// Arrange: Create a known timestamp (Epoch: 1970-01-01 00:00:00 UTC)
	// Note: Local time depends on machine timezone, so we can only check validity fields,
	// NOT exact hour (unless we force timezone env var, which is unsafe in unit tests).
	const auto epoch = std::chrono::system_clock::from_time_t(0);

	const std::tm result = toLocalTime(epoch);

	// Assert
	// Year is years since 1900. 70 means 1970.
	EXPECT_EQ(result.tm_year, 70);
	// Basic sanity checks
	EXPECT_GE(result.tm_mon, 0);
	EXPECT_LE(result.tm_mon, 11);
	EXPECT_GE(result.tm_mday, 1);
	EXPECT_LE(result.tm_mday, 31);
}