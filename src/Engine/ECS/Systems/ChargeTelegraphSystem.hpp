#pragma once
namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::ChargeTelegraphSystem {
void update(Registry &registry, float deltaTime);
}
