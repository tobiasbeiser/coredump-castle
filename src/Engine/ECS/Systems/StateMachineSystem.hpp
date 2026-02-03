#pragma once

#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems::StateMachineSystem {

void update(Registry &registry, float deltaTime);

} // namespace GameEngine::ECS::Systems::StateMachineSystem
