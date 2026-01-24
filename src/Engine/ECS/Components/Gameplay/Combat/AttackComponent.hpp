#pragma once
#include "Engine/Utils/Timer.hpp"

struct AttackComponent {
	Timer attackTimer{std::chrono::duration<float>::zero()};
	Timer attackCoolDownTimer{std::chrono::duration<float>::zero()};
	float windUpLimit = 0.0f; // percentage threshold when to stop the attack early. Needed for the
	                          // longer charge animation to recompute the raycast
};
