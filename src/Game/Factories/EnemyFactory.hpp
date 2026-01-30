#pragma once

#include "Engine/ECS/Entity.hpp"
#include "Game/Spawners/EnemySpawner.hpp"

#include <glm/vec2.hpp>

class AnimationFactory;

namespace tson { // NOLINT
class Map;
class Object;
} // namespace tson

namespace GameEngine::ECS {
class Registry;

class EnemyFactory {
  public:
	EnemyFactory(Registry &registry, AnimationFactory &animationFactory);

	// Delete copy/move constructors
	EnemyFactory(const EnemyFactory &) = delete;
	EnemyFactory &operator=(const EnemyFactory &) = delete;
	EnemyFactory(EnemyFactory &&) = delete;
	EnemyFactory &operator=(EnemyFactory &&) = delete;
	~EnemyFactory() = default;

	void setMap(tson::Map &tsonMap);

	EnemySpawner createEnemy(tson::Object &enemyObj, const glm::vec2 &levelPos, int layer) const;

  private:
	void ensureSafeSpawn(Entity entity) const;

	Registry &registry;
	AnimationFactory &animationFactory;
	tson::Map *map = nullptr;
};
} // namespace GameEngine::ECS