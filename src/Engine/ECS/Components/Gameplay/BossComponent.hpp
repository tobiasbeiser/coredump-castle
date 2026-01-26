#pragma once

#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"

struct BossComponent {
	int bossId = -1;
	AbilityType unlockAbility = AbilityType::None;
};
