#pragma once

#include "Engine/ECS/Registry.hpp"
#include "Engine/GameEngine.hpp"

namespace GameEngine::ECS::Systems::ReviveSystem{

void update(Registry &registry, GameEngine &game, const float deltaTime);

} // namespace GameEngine::ECS::Systems::RespawnSystem
