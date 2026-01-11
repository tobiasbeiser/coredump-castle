#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "Game/Factories/ProjectileFactory.hpp"

#include <cmath>
#include <numbers>

namespace GameEngine::ECS::Systems::ProjectileSystem {

namespace {
// Constants
constexpr float VERTICAL_SNAP_THRESHOLD = 20.0f;

struct PlayerTargetData {
	glm::vec2 centerPosition{0.0f, 0.0f};
	glm::vec2 cornerPosition{0.0f, 0.0f};
	bool isGrounded = false;
};

PlayerTargetData getPlayerTargetData(Registry &registry, bool facingLeft)
{
	PlayerTargetData playerData;

	for (auto playerView = registry.view<PlayerTag, CollisionComponent, PositionComponent, GroundedComponent>();
	     const auto player : playerView) {
		const auto &pos = playerView.get<PositionComponent>(player).position;
		const auto &collision = playerView.get<CollisionComponent>(player).collisionBox;
		playerData.isGrounded = playerView.get<GroundedComponent>(player).grounded;

		glm::vec2 playerScale{1.0f, 1.0f};
		if (registry.has<TransformComponent>(player)) {
			playerScale = registry.get<TransformComponent>(player).scale;
		}

		playerData.centerPosition.x = pos.x + (collision.x * playerScale.x) + (collision.w * playerScale.x * 0.5f);
		playerData.centerPosition.y = pos.y + (collision.y * playerScale.y) + (collision.h * playerScale.y * 0.5f);

		if (facingLeft) {
			playerData.cornerPosition.x = pos.x + (collision.x * playerScale.x) + (collision.w * playerScale.x);
			playerData.cornerPosition.y = pos.y + (collision.y * playerScale.y);
		} else {
			playerData.cornerPosition.x = pos.x + (collision.x * playerScale.x);
			playerData.cornerPosition.y = pos.y + (collision.y * playerScale.y);
		}
	}

	return playerData;
}

glm::vec2 calculateSpawnPosition(const glm::vec2 &enemyPosition, const SDL_FRect &enemyHurtBox,
                                 const glm::vec2 &enemyScale, const glm::vec2 &projectileOffset, bool facingLeft)
{
	glm::vec2 spawnPosition = enemyPosition;

	if (facingLeft) {
		spawnPosition.x += (enemyHurtBox.x * enemyScale.x) - projectileOffset.x;
	} else {
		spawnPosition.x += (enemyHurtBox.x * enemyScale.x) + (enemyHurtBox.w * enemyScale.x) + projectileOffset.x;
	}
	spawnPosition.y += projectileOffset.y;

	return spawnPosition;
}

glm::vec2 calculateProjectileDirection(const glm::vec2 &spawnPosition, const PlayerTargetData &playerData,
                                       const bool facingLeft)
{
	if (playerData.isGrounded && std::abs(playerData.centerPosition.y - spawnPosition.y) < VERTICAL_SNAP_THRESHOLD) {
		return {facingLeft ? -1.0f : 1.0f, 0.0f}; // Shoot horizontally in one straight line
	}

	return glm::normalize(playerData.cornerPosition - spawnPosition);
}

bool shouldSpawnProjectile(Registry &registry, Entity entity, const AttackComponent &attack, float deltaTime)
{
	if (registry.has<AnimationComponent>(entity)) {
		const auto &anim = registry.get<AnimationComponent>(entity);
		return anim.hasAnimation(AnimationType::Fire) && anim.getAnimation() == AnimationType::Fire;
	}

	return attack.attackTimer.getTime() <= std::chrono::duration<float>(deltaTime * 2.0f);
}

} // anonymous namespace

void update(Registry &registry, const float deltaTime, TextureManager &textureManager)
{
	for (auto projectileView =
	         registry.view<ProjectileComponent, VelocityComponent, PositionComponent>().exclude<AttackComponent>();
	     const auto entity : projectileView) {
		const auto &velocity = projectileView.get<VelocityComponent>(entity);
		auto &position = projectileView.get<PositionComponent>(entity);
		position.position += velocity.velocity * deltaTime;
	}

	const ProjectileFactory factory(registry, textureManager);

	// Spawn new projectiles from attacking enemies
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

			const PlayerTargetData playerData = getPlayerTargetData(registry, facingLeft);

			const glm::vec2 spawnPosition = calculateSpawnPosition(position.position, enemyHurtBox.hurtBox, enemyScale,
			                                                       rangedAttack.projectileOffset, facingLeft);
			const glm::vec2 direction = calculateProjectileDirection(spawnPosition, playerData, facingLeft);

			const double rotation = std::atan2(direction.y, direction.x) * (180.0 / std::numbers::pi);

			factory.createProjectile(spawnPosition, direction, projectileSettings.speed, projectileSettings.maxRange,
			                         projectileSettings.projectileTilesetPath, rotation);
			rangedAttack.projectileSpawned = true;
		}
	}

	// Clean up projectiles that have exceeded their max range
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

} // namespace GameEngine::ECS::Systems::ProjectileSystem