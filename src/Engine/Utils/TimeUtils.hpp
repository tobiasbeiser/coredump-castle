#pragma once
#include <chrono>
#include <ctime>

namespace GameEngine::Utils {

template <typename Clock, typename Duration>
[[nodiscard]] std::chrono::system_clock::time_point toSystemClock(std::chrono::time_point<Clock, Duration> timePoint)
{

	if constexpr (std::is_same_v<Clock, std::chrono::system_clock>) {
		return timePoint;
	} else {
		// TargetSysTime = CurrentSysTime + (TargetTime - CurrentClockTime)
		// This bypasses the need for knowing the specific Epoch of the source clock.
		auto sysNow = std::chrono::system_clock::now();
		auto clockNow = Clock::now();

		return sysNow + duration_cast<std::chrono::system_clock::duration>(timePoint - clockNow);
	}
}

[[nodiscard]] inline std::tm toLocalTime(const std::chrono::system_clock::time_point tp)
{
	std::time_t timeT = std::chrono::system_clock::to_time_t(tp);
	std::tm tm{};

// std::chrono support on apple is apparently an issue, can't avoid using macros here
#if defined(_WIN32)
	::localtime_s(&tm, &timeT);
#else
	::localtime_r(&timeT, &tm);
#endif

	return tm;
}
} // namespace GameEngine::Utils