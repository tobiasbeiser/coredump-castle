#pragma once
#include "Engine/Utils/Timer.hpp"

struct PoiseComponent {
	float currentPoise{0.0f};
	float maxPoise{0.0f};

	Timer recoveryTimer;
};
