#pragma once

#include "Engine/ECS/Registry.hpp"

namespace GameEngine::ECS::Systems::StateMachineSystem {

/**
 * Update all entities with state machines
 * - Increments timers
 * - Checks transitions
 * - Executes state actions
 */
void update(Registry &registry, float deltaTime);

} // namespace GameEngine::ECS::Systems::StateMachineSystem
