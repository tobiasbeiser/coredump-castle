#include "EnemySpawner.hpp"

#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/MapActivationSystem.hpp"
#include "Engine/Utils/ReferenceFrameUtils.hpp"
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
	const auto position = registry.get().tryGet<PositionComponent>(*currentEntity);
	const auto referenceFrame = GameEngine::Utils::ReferenceFrameUtils::getReferenceFrame(registry.get());
	if (!position || !GameEngine::Utils::ReferenceFrameUtils::isInFrame(position->get().position, referenceFrame)) {
		registry.get().registerComponent(*currentEntity, InactiveTag{});
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
