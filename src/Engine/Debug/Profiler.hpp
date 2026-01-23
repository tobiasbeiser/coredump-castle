#pragma once
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

struct SystemProfileData {
	std::vector<float> history;
	size_t offset = 0;
	std::chrono::duration<float> maxTime = {};

	SystemProfileData() { history.resize(60, 0.0f); }

	void addSample(const std::chrono::duration<float> duration)
	{
		history[offset] = duration.count();
		offset = (offset + 1) % history.size();
		if (duration > maxTime)
			maxTime = duration;
	}
};

class Profiler {
  public:
	static Profiler &instance()
	{
		static Profiler inst;
		return inst;
	}

	std::unordered_map<std::string, SystemProfileData> results;

	void update(const std::string &name, const std::chrono::duration<float> duration)
	{
		results[name].addSample(duration);
	}
};
