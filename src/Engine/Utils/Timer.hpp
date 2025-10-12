#pragma once

class Timer {
	float length, time;
	bool timeout = false;
public:
	Timer(float length) :length(length), time(0) {}

	void step(float deltaTime, bool loop)noexcept {
		time += deltaTime;
		if (loop) {
			if (time > length) {
				time -= length;
				timeout = true;
			}
		}
		else {
			if (time > length) {
				time = length;
				timeout = true;
			}
		}
	};
	bool isTimeout() const noexcept { return timeout; }
	float getTime() const noexcept { return time; }
	float getLength() const noexcept { return length; }
	void reset() noexcept { time = 0; timeout = false; }



};