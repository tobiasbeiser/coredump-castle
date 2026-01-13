#pragma once

class TextureManager;

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::ProjectileSystem {

void update(Registry &registry, float deltaTime, TextureManager &textureManager);

} // namespace GameEngine::ECS::Systems::ProjectileSystem