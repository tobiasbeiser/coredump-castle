#pragma once
#include "Engine/GameEngine.hpp"
#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems::DeathSystem {

void update(Registry& registry, GameEngine& game, float deltaTime);

} // namespace GameEngine::ECS::Systems::DeathSystem