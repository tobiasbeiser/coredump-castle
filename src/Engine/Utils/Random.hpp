#pragma once
#include <random>

namespace GameEngine::Utils::Random {

inline float getFloat(const float min, const float max)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dis(min, max);
	return dis(gen);
}

inline int getInt(const int min, const int max)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(min, max);
	return dis(gen);
}

} // namespace GameEngine::Utils::Random
