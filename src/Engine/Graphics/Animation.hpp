#pragma once
#include "../Utils/Timer.hpp"
#include <algorithm>

class Animation {
	Timer timer;
	int frameCount;
	bool loop = true;

  public:
	Animation() : timer(std::chrono::duration<float>::zero()), frameCount(0) {}
	Animation(const int frameCount, const std::chrono::duration<float> length, const bool loop = true) : timer(length), frameCount(frameCount), loop(loop) {}

	std::chrono::duration<float> getLength() const { return timer.getLength(); }
	int currentFrame() const
	{
		if (frameCount == 0) {
			return 0;
		}
		const std::chrono::duration<float> t = timer.getTime();
		if (!loop && t >= timer.getLength()) {
			return frameCount - 1;
		}
		const auto res = static_cast<int>((t / timer.getLength()) * frameCount);
		return std::clamp(res, 0, frameCount - 1);
	}

	bool completed() const { return timer.isTimeout(); };

	void step(const std::chrono::duration<float> deltaTime) { timer.step(deltaTime, loop); }
	void start() { timer.reset(); }
};