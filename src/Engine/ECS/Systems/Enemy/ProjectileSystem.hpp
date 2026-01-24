#pragma once

class TextureManager;

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::Utils {
class SpatialHash;
}

namespace GameEngine::ECS::Systems::ProjectileSystem {

void update(Registry &registry, float deltaTime, TextureManager &textureManager);

void handleEnemyProjectileCollisions(Registry &registry);

} // namespace GameEngine::ECS::Systems::ProjectileSystem