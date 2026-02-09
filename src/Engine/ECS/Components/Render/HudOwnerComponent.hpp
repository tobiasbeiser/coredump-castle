#pragma once

#include "Engine/ECS/Entity.hpp"

enum class HudOwnerType { Player, Boss, None };

struct HudOwnerComponent {
	HudOwnerType type = HudOwnerType::None;
	GameEngine::ECS::Entity trackedEntity = GameEngine::ECS::invalidEntity;
};