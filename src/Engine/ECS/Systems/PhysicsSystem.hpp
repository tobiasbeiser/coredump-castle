#pragma once

namespace GameEngine::Utils {
class SpatialHash;
}
namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::PhysicsSystem {

void update(Registry &registry, float timeStep);

} // namespace GameEngine::ECS::Systems::PhysicsSystem
