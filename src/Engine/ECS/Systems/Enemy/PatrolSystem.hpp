#pragma once

class TextureManager;

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::PatrolSystem {

void update(Registry &registry);

} // namespace GameEngine::ECS::Systems::PatrolSystem