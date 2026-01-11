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

void update(Registry &registry, const float deltaTime, TextureManager &textureManager)
{

	for (auto projectileView =
	         registry.view<ProjectileComponent, VelocityComponent, PositionComponent>().exclude<AttackComponent>();
	     const auto entity : projectileView) {
		const auto &velocity = projectileView.get<VelocityComponent>(entity);
		auto &position = projectileView.get<PositionComponent>(entity);

		position.position += velocity.velocity * deltaTime;
	}

	bool onGround = false;
	auto playerView = registry.view<PlayerTag, CollisionComponent, PositionComponent, GroundedComponent>();
	glm::vec2 playerCenterPosition{0.0f, 0.0f};
	glm::vec2 playerScale = {1.0f, 1.0f};

	for (auto player : playerView) {
		const auto &pos = playerView.get<PositionComponent>(player).position;
		const auto &collision = playerView.get<CollisionComponent>(player).collisionBox;
		if (registry.has<TransformComponent>(player)) {
			playerScale = registry.get<TransformComponent>(player).scale;
		}
		onGround = registry.get<GroundedComponent>(player).grounded;

		playerCenterPosition.x = pos.x + (collision.x * playerScale.x) + (collision.w * playerScale.x * 0.5f);
		playerCenterPosition.y = pos.y + (collision.y * playerScale.y) + (collision.h * playerScale.y * 0.5f);
	}

	// Spawn new projectiles
	for (auto view =
	         registry.view<EnemyTag, AttackComponent, RangedAttackComponent, PositionComponent, HurtBoxComponent>();
	     const auto entity : view) {
		auto &attack = registry.get<AttackComponent>(entity);
		auto &rangedAttack = registry.get<RangedAttackComponent>(entity);
		const auto &projectileSettings = registry.get<ProjectileComponent>(entity);
		const auto &position = registry.get<PositionComponent>(entity);
		const auto &enemyHurtBox = registry.get<HurtBoxComponent>(entity);

		if (!AttackSystem::isAttacking(attack)) {
			rangedAttack.projectileSpawned = false;
			continue;
		}

		if (rangedAttack.projectileSpawned) {
			continue;
		}

		bool shouldSpawn = false;
		if (registry.has<AnimationComponent>(entity)) {
			if (const auto &anim = registry.get<AnimationComponent>(entity); anim.hasAnimation(AnimationType::Fire)) {
				if (anim.getAnimation() == AnimationType::Fire) {
					shouldSpawn = true;
				}
			}
		} else {
			if (attack.attackTimer.getTime() <= std::chrono::duration<float>(deltaTime * 2.0f)) {
				shouldSpawn = true;
			}
		}

		if (!shouldSpawn) {
			continue;
		}

		glm::vec2 spawnPosition = position.position;
		glm::vec2 direction = {0.0f, 0.0f};

		glm::vec2 scale = {1.0f, 1.0f};
		if (registry.has<TransformComponent>(entity)) {
			scale = registry.get<TransformComponent>(entity).scale;
		}

		if (registry.has<RenderComponent>(entity)) {
			const auto dir = registry.get<RenderComponent>(entity).flipX;

			if (dir) {
				spawnPosition.x += (enemyHurtBox.hurtBox.x * scale.x) - rangedAttack.projectileOffset.x;
			} else {
				spawnPosition.x += (enemyHurtBox.hurtBox.x * scale.x) + (enemyHurtBox.hurtBox.w * scale.x)
				                   + rangedAttack.projectileOffset.x;
			}
			spawnPosition.y += rangedAttack.projectileOffset.y;

			if (constexpr float VERTICAL_SNAP_THRESHOLD = 20.0f;
			    onGround && std::abs(playerCenterPosition.y - spawnPosition.y) < VERTICAL_SNAP_THRESHOLD) {
				direction = {dir ? -1.0f : 1.0f, 0.0f};
			} else {
				direction = glm::normalize(playerCenterPosition - spawnPosition);
			}
		}

		const ProjectileFactory factory(registry, textureManager);

		double rotation = std::atan2(direction.y, direction.x) * (180.0 / std::numbers::pi);

		factory.createProjectile(spawnPosition, direction, projectileSettings.speed, projectileSettings.maxRange,
		                         rotation);
		rangedAttack.projectileSpawned = true;
	}

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