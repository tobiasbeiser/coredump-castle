#pragma once
#include "Profiler.hpp"

#include <chrono>
#include <string>

struct ScopedProfiler {
	std::string name;
	std::chrono::high_resolution_clock::time_point start;

	explicit ScopedProfiler(const std::string &name) : name(name), start(std::chrono::high_resolution_clock::now()) {}
	ScopedProfiler &operator=(const ScopedProfiler &) = delete;
	ScopedProfiler &operator=(const ScopedProfiler &&) = delete;
	ScopedProfiler(const ScopedProfiler &) = delete;
	ScopedProfiler(const ScopedProfiler &&) = delete;

	~ScopedProfiler()
	{
		const auto end = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<float> duration = std::chrono::duration<float, std::milli>(end - start);
		Profiler::instance().update(name, duration);
	}
};

#define PROFILE_SCOPE(name) ScopedProfiler timer##__LINE__(name)
#define PROFILE_FUNCTION() ScopedProfiler timer##__LINE__(__FUNCTION__)