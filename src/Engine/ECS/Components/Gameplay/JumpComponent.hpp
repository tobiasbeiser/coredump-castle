#pragma once
#include "Engine/Utils/Timer.hpp"

#include <chrono>

struct JumpComponent {
	float jumpStrength = 410;
	int jumpsLeft = 0;
	int maxJumps = 0;
	Timer coyoteTimer{std::chrono::duration<float>::zero()};
};
