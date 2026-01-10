#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Graphics/TextureManager.hpp"
#include "Game/Factories/AnimationFactory.hpp"
#include "Game/Factories/ProjectileFactory.hpp"

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
			direction = {dir ? -1.0f : 1.0f, 0.0f};
			if (dir) {
				spawnPosition.x += (enemyHurtBox.hurtBox.x * scale.x) - rangedAttack.projectileOffset.x;
			} else {
				spawnPosition.x += (enemyHurtBox.hurtBox.x * scale.x) + (enemyHurtBox.hurtBox.w * scale.x)
				                   + rangedAttack.projectileOffset.x;
			}
			spawnPosition.y += rangedAttack.projectileOffset.y;
		}

		const ProjectileFactory factory(registry, textureManager);

		factory.createProjectile(spawnPosition, direction, projectileSettings.speed, projectileSettings.maxRange);
		rangedAttack.projectileSpawned = true;
	}
}

} // namespace GameEngine::ECS::Systems::ProjectileSystem