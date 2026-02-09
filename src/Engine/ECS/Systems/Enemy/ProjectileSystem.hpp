#pragma once
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Entity.hpp"
#include "Game/Factories/ProjectileFactory.hpp"

class TextureManager;

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::Utils {
class SpatialHash;
}

namespace GameEngine::ECS::Systems {

class ProjectileSystem {
  public:
	explicit ProjectileSystem(ProjectileFactory &projectileFactory);
	ProjectileSystem(const ProjectileSystem &) = delete;
	ProjectileSystem(ProjectileSystem &&) = delete;
	ProjectileSystem &operator=(const ProjectileSystem &) = delete;
	ProjectileSystem &operator=(ProjectileSystem &&) = delete;
	~ProjectileSystem() = default;

	void update(Registry &registry, float deltaTime);
	static void handleEnemyProjectileCollisions(Registry &registry);

  private:
	void handleEnemySpawning(Registry &registry, float deltaTime) const;
	static void resolveProjectileWallCollisions(Registry &registry);
	static void cleanUpOldProjectiles(Registry &registry);
	static glm::vec2 calculateProjectileSpawnPosition(const glm::vec2 &enemyPosition, const SDL_FRect &enemyHurtBox,
	                                                  const glm::vec2 &enemyScale, const glm::vec2 &projectileOffset,
	                                                  bool facingLeft);

	static glm::vec2 calculateProjectileDirection(const glm::vec2 &projectileSpawnPosition,
	                                              const glm::vec2 &targetSnapshotPosition);

	static bool shouldSpawnProjectile(Registry &registry, Entity entity, const AttackComponent &attack,
	                                  float deltaTime);

	static void updateProjectileMovement(Registry &registry, float deltaTime);
	static bool isPlayerAlive(Registry &registry);
	static void cleanUpAllProjectiles(Registry &registry);

	ProjectileFactory &projectileFactory;
};

} // namespace GameEngine::ECS::Systems