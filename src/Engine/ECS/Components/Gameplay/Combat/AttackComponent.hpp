#pragma once
#include "Engine/Utils/Timer.hpp"

struct AttackComponent {
	Timer attackTimer{std::chrono::duration<float>::zero()};
	Timer attackCoolDownTimer{std::chrono::duration<float>::zero()};
};
