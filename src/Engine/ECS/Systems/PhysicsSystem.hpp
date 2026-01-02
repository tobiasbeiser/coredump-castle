#pragma once

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::PhysicsSystem {

void update(Registry &registry, float timeStep);

} // namespace GameEngine::ECS::Systems::PhysicsSystem
