#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "Game/Factories/ProjectileFactory.hpp"
#include <cmath>
#include <numbers>

namespace GameEngine::ECS::Systems::ProjectileSystem {

namespace {
glm::vec2 calculateProjectileSpawnPosition(const glm::vec2 &enemyPosition, const SDL_FRect &enemyHurtBox,
                                           const glm::vec2 &enemyScale, const glm::vec2 &projectileOffset,
                                           const bool facingLeft)
{
	glm::vec2 projectileSpawnPosition = enemyPosition;

	if (facingLeft) {
		projectileSpawnPosition.x += (enemyHurtBox.x * enemyScale.x) - projectileOffset.x;
	} else {
		projectileSpawnPosition.x += (enemyHurtBox.x * enemyScale.x) + projectileOffset.x;
	}
	projectileSpawnPosition.y += projectileOffset.y;

	return projectileSpawnPosition;
}

glm::vec2 calculateProjectileDirection(const glm::vec2 &projectileSpawnPosition,
                                       const glm::vec2 &targetSnapshotPosition)
{

	return glm::normalize(targetSnapshotPosition - projectileSpawnPosition);
}

bool shouldSpawnProjectile(Registry &registry, const Entity entity, const AttackComponent &attack,
                           const float deltaTime)
{
	if (registry.has<AnimationComponent>(entity)) {
		const auto &anim = registry.get<AnimationComponent>(entity);
		return anim.hasAnimation(AnimationType::Fire) && anim.getAnimation() == AnimationType::Fire;
	}

	return attack.attackTimer.getTime() <= std::chrono::duration<float>(deltaTime * 2.0f);
}
void updateProjectileMovement(Registry &registry, const float deltaTime)
{
	for (auto projectileView =
	         registry.view<ProjectileComponent, VelocityComponent, PositionComponent>().exclude<AttackComponent>();
	     const auto entity : projectileView) {
		const auto &velocity = projectileView.get<VelocityComponent>(entity);
		auto &position = projectileView.get<PositionComponent>(entity);
		position.previousPosition = position.position;
		position.position += velocity.velocity * deltaTime;
	}
}

void resolveProjectileWallCollisions(Registry &registry, const Utils::SpatialHash &grid)
{
	auto projectileView = registry.view<EnemyTag, PositionComponent, ProjectileComponent>();
	for (const auto entity : projectileView) {
		const auto &pos = registry.get<PositionComponent>(entity);
		const auto &projectile = registry.get<ProjectileComponent>(entity);

		if (CollisionSystem::isPathObstructed(pos.previousPosition, pos.position, grid, projectile.projectileSize)) {
			registry.deleteEntity(entity);
		}
	}
}

void cleanUpOldProjectiles(Registry &registry)
{
	auto projectileView =
	    registry.view<EnemyTag, PositionComponent, ProjectileComponent>().exclude<RangedAttackComponent>();
	for (auto entity : projectileView) {
		const auto &projectile = registry.get<ProjectileComponent>(entity);
		const auto &position = registry.get<PositionComponent>(entity);

		if (glm::distance(position.position, projectile.spawnPosition) >= projectile.maxRange) {
			registry.deleteEntity(entity);
		}
	}
}

bool isPlayerAlive(Registry &registry)
{
	auto playerView = registry.view<PlayerTag, StateComponent>();
	for (const auto entity : playerView) {
		const auto &state = registry.get<StateComponent>(entity);
		return state.state != State::Dead;
	}
	return false;
}

void cleanUpAllProjectiles(Registry &registry)
{
	auto projectileView = registry.view<EnemyTag, ProjectileComponent>().exclude<RangedAttackComponent>();
	for (auto entity : projectileView) {
		registry.deleteEntity(entity);
	}
}

void handleEnemySpawning(Registry &registry, float deltaTime, const ProjectileFactory &factory)
{
	for (auto view =
	         registry.view<EnemyTag, AttackComponent, RangedAttackComponent, PositionComponent, HurtBoxComponent>();
	     const auto entity : view) {
		auto &attack = registry.get<AttackComponent>(entity);
		auto &rangedAttack = registry.get<RangedAttackComponent>(entity);

		if (!AttackSystem::isAttacking(attack)) {
			rangedAttack.projectileSpawned = false;
			continue;
		}

		if (rangedAttack.projectileSpawned) {
			continue;
		}

		if (!shouldSpawnProjectile(registry, entity, attack, deltaTime)) {
			continue;
		}

		const auto &projectileSettings = registry.get<ProjectileComponent>(entity);
		const auto &position = registry.get<PositionComponent>(entity);
		const auto &enemyHurtBox = registry.get<HurtBoxComponent>(entity);

		glm::vec2 enemyScale{1.0f, 1.0f};
		if (registry.has<TransformComponent>(entity)) {
			enemyScale = registry.get<TransformComponent>(entity).scale;
		}

		if (registry.has<RenderComponent>(entity)) {
			const bool facingLeft = registry.get<RenderComponent>(entity).flipX;

			const glm::vec2 spawnPosition = calculateProjectileSpawnPosition(
			    position.position, enemyHurtBox.hurtBox, enemyScale, rangedAttack.projectileOffset, facingLeft);

			const glm::vec2 spawnCenter =
			    spawnPosition + projectileSettings.hitboxOffset + (projectileSettings.projectileSize * 0.5f);

			const glm::vec2 direction = calculateProjectileDirection(spawnCenter, rangedAttack.targetSnapshot);

			const double rotation = std::atan2(direction.y, direction.x) * (180.0 / std::numbers::pi);

			factory.createProjectile(spawnPosition, direction, projectileSettings.speed, projectileSettings.maxRange,
			                         projectileSettings.projectileTilesetPath, rotation);
			rangedAttack.projectileSpawned = true;
		}
	}
}
} // anonymous namespace

void handleEnemyProjectileCollisions(Registry &registry, const Utils::SpatialHash &grid)
{
	resolveProjectileWallCollisions(registry, grid);
}

void update(Registry &registry, const float deltaTime, TextureManager &textureManager)
{
	if (!isPlayerAlive(registry)) {
		cleanUpAllProjectiles(registry);
		return;
	}

	updateProjectileMovement(registry, deltaTime);

	const ProjectileFactory factory(registry, textureManager);

	handleEnemySpawning(registry, deltaTime, factory);

	cleanUpOldProjectiles(registry);
}

} // namespace GameEngine::ECS::Systems::ProjectileSystem