#pragma once
#include "Engine/Utils/Timer.hpp"

struct JumpComponent {
	float jumpStrength = 410;
	int jumpsLeft = 0;
	int maxJumps = 0;
	Timer coyoteTimer{0.0f};
};
