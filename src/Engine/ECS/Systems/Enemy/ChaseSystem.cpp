#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems::ChaseSystem {

void update(Registry &registry, float deltaTime)
{
	auto enemyView =
	    registry.view<EnemyTag, InputComponent, PatrolComponent, ChaseComponent, PositionComponent, HitBoxComponent,
	                  HurtBoxComponent, CollisionComponent, MovementComponent, GroundedComponent, AttackComponent,
	                  RenderComponent>();
	auto playerView =
	    registry.view<PlayerTag, PositionComponent, CollisionComponent, VelocityComponent, TransformComponent>();

	glm::vec2 playerCenterPos = {0, 0};
	SDL_FRect playerHurtBox = {0, 0, 0, 0};
	glm::vec2 playerScale = {1.0f, 1.0f};

	for (const auto player : playerView) {
		const auto &playerPos = registry.get<PositionComponent>(player).position;
		playerHurtBox = registry.get<HurtBoxComponent>(player).hurtBox;
		playerScale = registry.get<TransformComponent>(player).scale;
		playerCenterPos =
		    glm::vec2(playerPos.x + (playerHurtBox.x * playerScale.x) + (playerHurtBox.w * playerScale.x) / 2.0f,
		              playerPos.y + (playerHurtBox.y * playerScale.y) + (playerHurtBox.h * playerScale.y) / 2.0f);
	}

	for (const auto enemy : enemyView) {
		const auto &enemyPosition = registry.get<PositionComponent>(enemy);
		auto &enemyInput = registry.get<InputComponent>(enemy);
		const auto &enemyHitBox = registry.get<HitBoxComponent>(enemy);
		const auto &enemyPatrol = registry.get<PatrolComponent>(enemy);
		auto &enemyChase = registry.get<ChaseComponent>(enemy);
		const auto &enemyHurtBox = registry.get<HurtBoxComponent>(enemy).hurtBox;
		auto &enemyMovement = registry.get<MovementComponent>(enemy);
		const auto &attack = registry.get<AttackComponent>(enemy);
		auto &enemyRender = registry.get<RenderComponent>(enemy);
		glm::vec2 enemyScale = {1.0f, 1.0f};
		if (registry.has<TransformComponent>(enemy)) {
			enemyScale = registry.get<TransformComponent>(enemy).scale;
		}

		enemyChase.reactionDelay.step(std::chrono::duration<float>(deltaTime), false);

		if (const bool isInTerritory =
		        playerCenterPos.x >= enemyPatrol.minBounds.x && playerCenterPos.x <= enemyPatrol.maxBounds.x;
		    !isInTerritory) {
			enemyInput.attackPressed = false;
			enemyInput.jumpPressed = false;
			enemyMovement.maxSpeedX = enemyChase.originalPatrolSpeed;
			enemyChase.reactionDelay.reset();
			continue;
		}

		const auto enemyCenterPos = glm::vec2(
		    enemyPosition.position.x + (enemyHurtBox.x * enemyScale.x) + (enemyHurtBox.w * enemyScale.x) / 2.0f,
		    enemyPosition.position.y + (enemyHurtBox.y * enemyScale.y) + (enemyHurtBox.h * enemyScale.y) / 2.0f);

		if (const float yDifference = playerCenterPos.y - enemyCenterPos.y;
		    std::abs(yDifference) > enemyChase.verticalDetectionHeight) {
			enemyInput.attackPressed = false;
			enemyMovement.maxSpeedX = enemyChase.originalPatrolSpeed;
			enemyChase.reactionDelay.reset();
			continue;
		}

		const float xDistance = std::abs(playerCenterPos.x - enemyCenterPos.x);
		const float directionToPlayer = playerCenterPos.x > enemyCenterPos.x ? 1 : -1;

		const float distanceToCheck = (playerHurtBox.w * playerScale.x / 2.0f) + (enemyHurtBox.w * enemyScale.x / 2.0f)
		                              + (enemyHitBox.weaponHitBox.w * enemyScale.x);

		if (AttackSystem::isAttacking(attack)) {
			enemyInput.attackPressed = false;
			enemyInput.horizontalDirection = 0.0f;
			continue;
		}

		if (xDistance <= distanceToCheck) {
			enemyInput.horizontalDirection = 0.0f;
			enemyRender.flipX = directionToPlayer < 0;

			if (enemyChase.reactionDelay.isTimeout()) {
				enemyInput.attackPressed = true;
			} else {
				enemyInput.attackPressed = false;
			}
		} else {
			enemyInput.horizontalDirection = directionToPlayer;
			enemyMovement.maxSpeedX = enemyChase.chaseSpeed;
			enemyInput.attackPressed = false;
			enemyChase.reactionDelay.reset();
		}
	}
}

} // namespace GameEngine::ECS::Systems::ChaseSystem
