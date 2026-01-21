#pragma once
#include "Engine/Utils/Timer.hpp"
struct PendingRespawnComponent {
	Timer delay{std::chrono::milliseconds(750)};
};