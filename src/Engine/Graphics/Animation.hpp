#pragma once
#include "../Utils/Timer.hpp"
#include <algorithm>

class Animation {
	Timer timer;
	int frameCount;
	bool loop = true;

  public:
	Animation() : timer(0), frameCount(0) {}
	Animation(int frameCount, float length, bool loop = true) : timer(length), frameCount(frameCount), loop(loop) {}

	float getLength() const { return timer.getLength(); }
	int currentFrame() const
	{
		if (frameCount == 0) {
			return 0;
		}
		const float t = timer.getTime();
		if (!loop && t >= timer.getLength()) {
			return frameCount - 1;
		}
		auto res = static_cast<int>((t / timer.getLength()) * frameCount);
		return std::clamp(res, 0, frameCount - 1);
	}

	void step(float deltaTime) { timer.step(deltaTime, loop); }
	void start() { timer.reset(); }
};