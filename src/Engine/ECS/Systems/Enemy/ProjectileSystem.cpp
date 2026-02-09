#include "ProjectileSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/ChargeTelegraphComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Components/Tags/ProjectileTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Game/AI/Actions/CoreActions.hpp"
#include "Game/Factories/ProjectileFactory.hpp"
#include <chrono>
#include <cmath>
#include <glm/geometric.hpp>
#include <numbers>

glm::vec2 GameEngine::ECS::Systems::ProjectileSystem::calculateProjectileSpawnPosition(
    const glm::vec2 &enemyPosition, const SDL_FRect &enemyHurtBox, const glm::vec2 &enemyScale,
    const glm::vec2 &projectileOffset, const bool facingLeft)
{
	glm::vec2 projectileSpawnPosition = enemyPosition;

	if (facingLeft) {
		projectileSpawnPosition.x += (enemyHurtBox.x * enemyScale.x) - projectileOffset.x;
	} else {
		projectileSpawnPosition.x += ((enemyHurtBox.x + enemyHurtBox.w) * enemyScale.x) + projectileOffset.x;
	}
	projectileSpawnPosition.y += (enemyHurtBox.y * enemyScale.y) + projectileOffset.y;

	return projectileSpawnPosition;
}

glm::vec2
GameEngine::ECS::Systems::ProjectileSystem::calculateProjectileDirection(const glm::vec2 &projectileSpawnPosition,
                                                                         const glm::vec2 &targetSnapshotPosition)
{

	return glm::normalize(targetSnapshotPosition - projectileSpawnPosition);
}

bool GameEngine::ECS::Systems::ProjectileSystem::shouldSpawnProjectile(Registry &registry, const Entity entity,
                                                                       const AttackComponent &attack,
                                                                       const float deltaTime)
{
	if (const auto anim = registry.tryGet<AnimationComponent>(entity)) {
		return anim->get().hasAnimation(AnimationType::Fire) && anim->get().getAnimation() == AnimationType::Fire;
	}

	return attack.attackTimer.getTime() <= std::chrono::duration<float>(deltaTime * 2.0f);
}
void GameEngine::ECS::Systems::ProjectileSystem::updateProjectileMovement(Registry &registry, const float deltaTime)
{
	auto projectileView = registry.view<ProjectileTag, ProjectileComponent, VelocityComponent, PositionComponent>();
	for (const auto entity : projectileView) {
		const auto &velocity = projectileView.get<VelocityComponent>(entity);
		auto &position = projectileView.get<PositionComponent>(entity);
		position.previousPosition = position.position;
		position.position += velocity.velocity * deltaTime;
	}
}

void GameEngine::ECS::Systems::ProjectileSystem::resolveProjectileWallCollisions(Registry &registry)
{
	auto projectileView = registry.view<ProjectileTag, PositionComponent, CollisionComponent>();
	for (const auto entity : projectileView) {
		const auto &pos = projectileView.get<PositionComponent>(entity);
		const auto &collision = projectileView.get<CollisionComponent>(entity);
		const glm::vec2 projectileSize = {collision.collisionBox.w, collision.collisionBox.h};

		if (CollisionSystem::isPathObstructed(pos.previousPosition, pos.position, projectileSize)) {
			registry.deleteEntity(entity);
		}
	}
}

void GameEngine::ECS::Systems::ProjectileSystem::cleanUpOldProjectiles(Registry &registry)
{
	auto projectileView = registry.view<ProjectileTag, PositionComponent, ProjectileComponent>();
	for (auto entity : projectileView) {
		const auto &projectile = projectileView.get<ProjectileComponent>(entity);
		const auto &position = projectileView.get<PositionComponent>(entity);

		if (glm::distance(position.position, projectile.spawnPosition) >= projectile.maxRange) {
			registry.deleteEntity(entity);
		}
	}
}

bool GameEngine::ECS::Systems::ProjectileSystem::isPlayerAlive(Registry &registry)
{
	auto playerView = registry.view<PlayerTag, StateComponent>();
	for (const auto entity : playerView) {
		const auto &state = playerView.get<StateComponent>(entity);
		return state.state != State::Dead;
	}
	return false;
}

void GameEngine::ECS::Systems::ProjectileSystem::cleanUpAllProjectiles(Registry &registry)
{
	auto projectileView = registry.view<ProjectileTag, ProjectileComponent>();
	for (auto entity : projectileView) {
		registry.deleteEntity(entity);
	}
}

void GameEngine::ECS::Systems::ProjectileSystem::handleEnemySpawning(Registry &registry, float deltaTime) const
{
	auto view = registry.view<EnemyTag, AttackComponent, RangedAttackComponent, PositionComponent, HurtBoxComponent,
	                          ProjectileComponent>();
	for (const auto entity : view) {
		auto &attack = view.get<AttackComponent>(entity);
		auto &rangedAttack = view.get<RangedAttackComponent>(entity);

		const auto isSpreadTelegraphActive = [&rangedAttack]() {
			return !rangedAttack.spreadTelegraphTimer.isTimeout();
		};

		rangedAttack.spreadCooldownTimer.step(std::chrono::duration<float>(deltaTime), false);
		if (isSpreadTelegraphActive()) {
			rangedAttack.spreadTelegraphTimer.step(std::chrono::duration<float>(deltaTime), false);
		}

		if (!AttackSystem::isAttacking(attack)) {
			rangedAttack.projectileSpawned = false;
			if (isSpreadTelegraphActive()) {
				rangedAttack.spreadTelegraphTimer.timeoutNow();
				if (const auto telegraphOpt = registry.tryGet<ChargeTelegraphComponent>(entity)) {
					telegraphOpt->get().active = false;
				}
			}
			continue;
		}

		if (rangedAttack.projectileSpawned) {
			continue;
		}

		if (!shouldSpawnProjectile(registry, entity, attack, deltaTime)) {
			continue;
		}

		const auto &projectileSettings = view.get<ProjectileComponent>(entity);
		const auto &position = view.get<PositionComponent>(entity);
		const auto &enemyHurtBox = view.get<HurtBoxComponent>(entity);

		const auto transform = registry.tryGet<TransformComponent>(entity);
		const glm::vec2 enemyScale = transform ? transform->get().scale : glm::vec2{1.0f, 1.0f};

		if (const auto render = registry.tryGet<RenderComponent>(entity)) {
			const auto &renderComp = render->get();
			bool facingLeft = renderComp.isFacingLeft();

			auto playerView = registry.view<PlayerTag, PositionComponent>();
			for (const auto playerEntity : playerView) {
				const auto &playerPos = playerView.get<PositionComponent>(playerEntity);
				facingLeft = (playerPos.position.x - position.position.x) < 0.0f;
				render->get().setFacingLeft(facingLeft);
				break;
			}

			const glm::vec2 spawnPosition = calculateProjectileSpawnPosition(
			    position.position, enemyHurtBox.hurtBox, enemyScale, rangedAttack.projectileOffset, facingLeft);

			glm::vec2 hitboxOffset = {0.0f, 0.0f};
			glm::vec2 projectileSize = {0.0f, 0.0f};
			if (!projectileSettings.projectileTilesetPath.empty()) {
				const auto &projectileData =
				    ProjectileFactory::getCachedProjectileData(projectileSettings.projectileTilesetPath);
				const glm::vec2 pScale = projectileData.scale;
				hitboxOffset = {projectileData.hitbox.x * pScale.x, projectileData.hitbox.y * pScale.y};
				projectileSize = {projectileData.hitbox.w * pScale.x, projectileData.hitbox.h * pScale.y};
			}

			const glm::vec2 spawnCenter = spawnPosition + hitboxOffset + (projectileSize * 0.5f);

			const glm::vec2 baseDirection = calculateProjectileDirection(spawnCenter, rangedAttack.targetSnapshot);
			const double baseRotation = std::atan2(baseDirection.y, baseDirection.x);

			bool useSpreadShot = false;
			bool spreadCandidate = false;
			if (rangedAttack.useSpreadShot && rangedAttack.spreadCount > 1) {
				bool farEnough = true;
				if (rangedAttack.spreadMinRange > 0.0f) {
					const auto enemyInfo = getEntityInfo(registry, entity);
					const auto playerInfo = getPlayerInfo(registry);
					if (enemyInfo && playerInfo) {
						const float distance = glm::distance(enemyInfo->center, playerInfo->center);
						farEnough = distance >= rangedAttack.spreadMinRange;
					} else {
						farEnough = false;
					}
				}

				const bool cooldownReady = rangedAttack.spreadCooldownTimer.isTimeout();
				spreadCandidate = farEnough && cooldownReady;
			}



			if (!rangedAttack.spreadTelegraphTimer.isTimeout()) {
					continue;
			}
			if (rangedAttack.spreadTelegraphTimer.isTimeout()) {
				useSpreadShot = spreadCandidate;
			}

			if (useSpreadShot) {
				const int count = rangedAttack.spreadCount;
				const float angleStep = rangedAttack.spreadAngle * (std::numbers::pi_v<float> / 180.0f);
				const float startAngle = baseRotation - (angleStep * (count - 1) / 2.0f);

				for (int i = 0; i < count; ++i) {
					const float angle = startAngle + (angleStep * i);
					const glm::vec2 direction = {std::cos(angle), std::sin(angle)};
					const double rotationDeg = angle * (180.0 / std::numbers::pi_v<float>);

					projectileFactory.createProjectile(spawnPosition, direction, projectileSettings.speed,
					                                   projectileSettings.maxRange,
					                                   projectileSettings.projectileTilesetPath, rotationDeg);
				}

				rangedAttack.spreadCooldownTimer.reset();
			} else {
				const double rotation = baseRotation * (180.0 / std::numbers::pi_v<float>);
				projectileFactory.createProjectile(spawnPosition, baseDirection, projectileSettings.speed,
				                                   projectileSettings.maxRange,
				                                   projectileSettings.projectileTilesetPath, rotation);
			}

			rangedAttack.projectileSpawned = true;
		}
	}
}

void GameEngine::ECS::Systems::ProjectileSystem::handleEnemyProjectileCollisions(Registry &registry)
{
	resolveProjectileWallCollisions(registry);
}

GameEngine::ECS::Systems::ProjectileSystem::ProjectileSystem(ProjectileFactory &projectileFactory)
    : projectileFactory(projectileFactory)
{
}
void GameEngine::ECS::Systems::ProjectileSystem::update(Registry &registry, const float deltaTime)
{
	if (!isPlayerAlive(registry)) {
		cleanUpAllProjectiles(registry);
		return;
	}

	updateProjectileMovement(registry, deltaTime);

	handleEnemySpawning(registry, deltaTime);

	cleanUpOldProjectiles(registry);
}
