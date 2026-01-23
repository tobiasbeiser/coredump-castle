#pragma once
#include "Profiler.hpp"

#include <chrono>
#include <string>

#pragma once
#include "Profiler.hpp"

#include <atomic>
#include <chrono>
#include <string>

struct ScopedProfiler {
	std::string name;
	uint64_t instanceId;
	std::chrono::high_resolution_clock::time_point start;

	explicit ScopedProfiler(const std::string& name)
		: name(name),
		  start(std::chrono::high_resolution_clock::now()) {}

	ScopedProfiler(const ScopedProfiler&) = delete;
	ScopedProfiler(ScopedProfiler&&) = delete;
	ScopedProfiler& operator=(const ScopedProfiler&) = delete;
	ScopedProfiler& operator=(ScopedProfiler&&) = delete;

	~ScopedProfiler() {
		const auto end = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<float, std::milli> duration(end - start);
		Profiler::instance().update(name, duration);
	}
};

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
// helper macros
#define PROFILE_CONCAT_INTERNAL(x, y) x##y
#define PROFILE_CONCAT(x, y) PROFILE_CONCAT_INTERNAL(x, y)

// public macros
#define PROFILE_SCOPE(name) ScopedProfiler PROFILE_CONCAT(timer_, __COUNTER__)(name)
#define PROFILE_FUNCTION() ScopedProfiler PROFILE_CONCAT(timer_, __COUNTER__)(__FUNCTION__)
// NOLINTEND(cppcoreguidelines-macro-usage)
