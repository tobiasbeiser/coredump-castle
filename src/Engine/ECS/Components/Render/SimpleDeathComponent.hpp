#pragma once
#include "Engine/Utils/Timer.hpp"

#include <chrono>
#include <ratio>

struct SimpleDeathComponent {
	Timer deathTimer{std::chrono::duration<float, std::milli>(1500)};
};