#pragma once
#include "Engine/ECS/Registry.hpp"
#include "Engine/GameEngine.hpp"

namespace GameEngine::ECS::Systems::RespawnSystem {

void update(Registry &registry, GameEngine &game);

} // namespace GameEngine::ECS::Systems::RespawnSystem