#include "EnemySpawner.hpp"

#include "Engine/ECS/Registry.hpp"
EnemySpawner::EnemySpawner(GameEngine::ECS::Registry &registry) : registry(registry), currentEntity(registry.create())
{
}
void EnemySpawner::spawn() {}
void EnemySpawner::despawn() {}
bool EnemySpawner::isAlive() const {}

template <typename T>
void EnemySpawner::addBlueprint(T component)
{
	blueprints.emplace_back([this, component](GameEngine::ECS::Entity e) { registry.registerComponent(e, component); });
}
