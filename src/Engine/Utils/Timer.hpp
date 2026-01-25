#pragma once
#include <chrono>

class Timer {
	std::chrono::duration<float> length, time{0};
	bool timeout = false;
	bool started = false;

  public:
	explicit Timer(const std::chrono::duration<float> length, const bool startTimedOut = false)
	    : length(length), timeout(startTimedOut)
	{
		if (startTimedOut) {
			time = length;
		}
	}

	[[nodiscard]] bool isRunning() const noexcept { return started; }

	void step(const std::chrono::duration<float> deltaTime, const bool loop) noexcept
	{
		started = true;
		time += deltaTime;
		if (loop) {
			if (time >= length) {
				time -= length;
				timeout = true;
			}
		} else {
			if (time >= length) {
				time = length;
				timeout = true;
			}
		}
	};
	[[nodiscard]] bool isTimeout() const noexcept { return timeout; }
	[[nodiscard]] std::chrono::duration<float> getTime() const noexcept { return time; }
	[[nodiscard]] std::chrono::duration<float> getLength() const noexcept { return length; }

	void reset() noexcept
	{
		time = std::chrono::duration<float>::zero();
		timeout = false;
		started = false;
	}

	void timeoutNow() noexcept
	{
		time = length;
		timeout = true;
	}
};