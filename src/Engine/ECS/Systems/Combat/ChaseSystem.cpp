#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"

#include <iostream>

namespace GameEngine::ECS::Systems::ChaseSystem {

void update(Registry &registry)
{
	auto enemyView =
	    registry.view<EnemyTag, InputComponent, PatrolComponent, ChaseComponent, PositionComponent, HitBoxComponent,
	                  HurtBoxComponent, CollisionComponent, MovementComponent, GroundedComponent>();
	auto playerView = registry.view<PlayerTag, PositionComponent, CollisionComponent, VelocityComponent>();

	glm::vec2 playerCenterPos = {0, 0};
	glm::vec2 playerVelocity = {0, 0};
	SDL_FRect playerHurtBox = {0, 0, 0, 0};

	for (const auto player : playerView) {
		const auto &playerPos = registry.get<PositionComponent>(player).position;
		playerHurtBox = registry.get<HurtBoxComponent>(player).hurtBox;
		playerVelocity = registry.get<VelocityComponent>(player).velocity;

		playerCenterPos = glm::vec2(playerPos.x + playerHurtBox.x + playerHurtBox.w / 2.0f,
		                            playerPos.y + playerHurtBox.y + playerHurtBox.h / 2.0f);
	}

	for (const auto enemy : enemyView) {
		const auto &enemyPosition = registry.get<PositionComponent>(enemy);
		auto &enemyInput = registry.get<InputComponent>(enemy);
		const auto &enemyHitBox = registry.get<HitBoxComponent>(enemy);
		const auto &enemyPatrol = registry.get<PatrolComponent>(enemy);
		const auto &enemyChase = registry.get<ChaseComponent>(enemy);
		const auto &enemyHurtBox = registry.get<HurtBoxComponent>(enemy).hurtBox;
		auto &enemyMovment = registry.get<MovementComponent>(enemy);
		const auto &enemyGrounded = registry.get<GroundedComponent>(enemy);

		if (const bool isInTerritory =
		        playerCenterPos.x >= enemyPatrol.minBounds.x && playerCenterPos.x <= enemyPatrol.maxBounds.x;
		    !isInTerritory) {
			enemyInput.attackPressed = false;
			enemyInput.jumpPressed = false;
			enemyMovment.maxSpeedX = enemyChase.originalPatrolSpeed;
			continue;
		}

		const auto enemyCenterPos = glm::vec2(enemyPosition.position.x + enemyHurtBox.x + enemyHurtBox.w / 2.0f,
		                                      enemyPosition.position.y + enemyHurtBox.y + enemyHurtBox.h / 2.0f);

		const float yDifference = playerCenterPos.y - enemyCenterPos.y; // Positive if player is below

		if (std::abs(yDifference) > enemyChase.verticalDetectionHeight) {
			enemyInput.attackPressed = false;
			enemyMovment.maxSpeedX = enemyChase.originalPatrolSpeed;
			continue;
		}

		const float xDistance = std::abs(playerCenterPos.x - enemyCenterPos.x);
		const float directionToPlayer = playerCenterPos.x > enemyCenterPos.x ? 1 : -1;


		float distanceToCheck = enemyHitBox.hitbox.w;
		if (directionToPlayer == 1) {
			distanceToCheck += playerHurtBox.w + enemyHurtBox.w;
		}

		if (xDistance <= distanceToCheck) {
			enemyInput.horizontalDirection = 0.0f;
			enemyInput.attackPressed = true;
		} else {
			enemyInput.horizontalDirection = directionToPlayer;
			enemyMovment.maxSpeedX = enemyChase.chaseSpeed;
			enemyInput.attackPressed = false;
		}
	}
}

} // namespace GameEngine::ECS::Systems::ChaseSystem
