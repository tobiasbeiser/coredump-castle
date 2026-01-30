#include "EnemySpawner.hpp"

#include "Engine/ECS/Registry.hpp"
EnemySpawner::EnemySpawner(GameEngine::ECS::Registry &registry) : registry(registry), currentEntity(registry.create())
{
}
void EnemySpawner::spawn()
{
	if (currentEntity) {
		despawn();
	}
	currentEntity = registry.get().create();
	for (const auto &blueprint : blueprints) {
		blueprint(*currentEntity);
	}
}
void EnemySpawner::despawn() const
{
	if (currentEntity) {
		registry.get().deleteEntity(*currentEntity);
	}
}
bool EnemySpawner::isAlive() const
{
	return currentEntity && registry.get().exists(*currentEntity);
}
