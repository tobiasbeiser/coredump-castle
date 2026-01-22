#pragma once

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::EffectSystem {

void update(Registry &registry, float dt);

} // namespace GameEngine::ECS::Systems::EffectSystem
