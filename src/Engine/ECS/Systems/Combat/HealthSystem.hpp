#pragma once

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::HealthSystem {

void update(Registry &registry, float deltaTime);

} // namespace GameEngine::ECS::Systems::HealthSystem