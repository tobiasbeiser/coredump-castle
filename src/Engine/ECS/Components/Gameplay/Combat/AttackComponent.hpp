#pragma once
#include "Engine/Utils/Timer.hpp"

struct AttackComponent {
	Timer attackTimer{0.0f};
	bool isAttacking = false;
	Timer attackCoolDownTimer{0.0f};
};
