#pragma once
#include "../Utils/Timer.hpp"

class Animation {
	Timer timer;
	int frameCount;
	bool loop = true;

public:
	Animation()  : timer(0), frameCount(0)  {}
	Animation(int frameCount, float length, bool loop = true) : timer(length), frameCount(frameCount), loop(loop) {}

	float getLength() const  { return timer.getLength(); }
	int currentFrame() const  {
		const float t = timer.getTime();
		if (!loop && t >= timer.getLength()) {
			return frameCount - 1;
		}
		return static_cast<int>((t / timer.getLength()) * frameCount);
	}


	void step(float deltaTime)  { timer.step(deltaTime, loop); }
	void start()  { timer.reset(); }
};