#include "EnemySpawner.hpp"

#include "Engine/ECS/Components/Gameplay/ProgressComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/MapActivationSystem.hpp"
#include "Engine/Utils/ReferenceFrameUtils.hpp"
EnemySpawner::EnemySpawner(GameEngine::ECS::Registry &registry) : registry(registry), currentEntity(registry.create())
{
}
void EnemySpawner::spawn()
{
	if (bossId) {
		auto playerView = registry.get().view<PlayerTag, ProgressComponent>();
		if (const auto &progress = playerView.get<ProgressComponent>(playerView.at(0));
		    progress.isBossDefeated(*bossId)) {
			return;
		}
	}

	if (currentEntity) {
		despawn();
	}
	currentEntity = registry.get().create();
	for (const auto &blueprint : blueprints) {
		blueprint(*currentEntity);
	}
	auto position = registry.get().tryGet<PositionComponent>(*currentEntity);
	const auto sprite = registry.get().tryGet<SpriteComponent>(*currentEntity);
	if (sprite && position) {
		position->get().position.x = position->get().position.x - sprite->get().tileSize.x / 2.0f;
		position->get().position.y = position->get().position.y - sprite->get().tileSize.y;
	}

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
void EnemySpawner::setBossId(int const id)
{
	bossId = id;
}
