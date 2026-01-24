#pragma once

#include <chrono>
#include <string>

#include "Profiler.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

struct ScopedProfiler {
	std::string name;
	std::chrono::high_resolution_clock::time_point start;

	explicit ScopedProfiler(const std::string &name_) : name(name_), start(std::chrono::high_resolution_clock::now()) {}

	ScopedProfiler(const ScopedProfiler &) = delete;
	ScopedProfiler(ScopedProfiler &&) = delete;
	ScopedProfiler &operator=(const ScopedProfiler &) = delete;
	ScopedProfiler &operator=(ScopedProfiler &&) = delete;

	~ScopedProfiler()
	{
		const auto end = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<float, std::milli> duration(end - start);
		Profiler::instance().update(name, duration);
	}
};

template <typename T, T...>
struct ConcatHelper;

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#define CONCAT(x, y) x##y

#ifdef TRACY_ENABLE
#define PROFILE_SCOPE(name) \
	ScopedProfiler CONCAT(timer_, __COUNTER__)(name); \
	ZoneScopedN(name)
#define PROFILE_FUNCTION() \
	ScopedProfiler CONCAT(timer_, __COUNTER__)(__FUNCTION__); \
	ZoneScoped
#define PROFILE_FRAME() FrameMark
#else
#define PROFILE_SCOPE(name) ScopedProfiler CONCAT(timer_, __COUNTER__)(name)
#define PROFILE_FUNCTION() ScopedProfiler CONCAT(timer_, __COUNTER__)(__FUNCTION__)
#define PROFILE_FRAME()
#endif
// NOLINTEND(cppcoreguidelines-macro-usage)