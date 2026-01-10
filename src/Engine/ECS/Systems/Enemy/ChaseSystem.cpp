#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"

namespace GameEngine::ECS::Systems::ChaseSystem {

namespace {

void resetChaseState(InputComponent &input, MovementComponent &movement, ChaseComponent &chase)
{
	input.attackPressed = false;
	movement.maxSpeedX = chase.originalPatrolSpeed;
	chase.reactionDelay.reset();
}

void handleRangeAttack(glm::vec2 enemyCenter, glm::vec2 playerCenter, ChaseComponent &chase,
                       const RangedAttackComponent &rangedAttack, InputComponent &input, RenderComponent &render,
                       const AttackComponent &attack)
{
	float distX = std::abs(playerCenter.x - enemyCenter.x);
	float distY = std::abs(playerCenter.y - enemyCenter.y);
	const float directionToPlayer = playerCenter.x > enemyCenter.x ? 1.0f : -1.0f;

	if (!AttackSystem::isAttacking(attack)
	    && (distX > rangedAttack.detectionRange || distY > rangedAttack.detectionRange)) {
		input.attackPressed = false;
		chase.reactionDelay.reset();
		return;
	}

	// player is close enough
	chase.lastKnownPlayerX = playerCenter.x;
	input.horizontalDirection = 0.0f;
	render.flipX = directionToPlayer < 0;
	input.attackPressed = chase.reactionDelay.isTimeout();
}

} // namespace

void update(Registry &registry, const float deltaTime)
{
	auto playerView =
	    registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent, GroundedComponent>();

	glm::vec2 playerCenter = {0, 0};
	SDL_FRect playerHurtBox = {0, 0, 0, 0};
	glm::vec2 playerScale = {1.0f, 1.0f};
	bool playerGrounded = false;

	for (const auto entity : playerView) {
		const auto &pos = registry.get<PositionComponent>(entity).position;
		playerHurtBox = registry.get<HurtBoxComponent>(entity).hurtBox;
		playerScale = registry.get<TransformComponent>(entity).scale;
		playerGrounded = registry.get<GroundedComponent>(entity).grounded;

		playerCenter = glm::vec2(pos.x + (playerHurtBox.x * playerScale.x) + (playerHurtBox.w * playerScale.x) * 0.5f,
		                         pos.y + (playerHurtBox.y * playerScale.y) + (playerHurtBox.h * playerScale.y) * 0.5f);
		break;
	}

	auto enemyView = registry.view<EnemyTag, InputComponent, PatrolComponent, ChaseComponent, PositionComponent,
	                               HitBoxComponent, HurtBoxComponent, CollisionComponent, MovementComponent,
	                               GroundedComponent, AttackComponent, RenderComponent>();

	for (const auto entity : enemyView) {
		const auto &pos = registry.get<PositionComponent>(entity);
		const auto &enemyHurtBox = registry.get<HurtBoxComponent>(entity).hurtBox;
		const auto &patrol = registry.get<PatrolComponent>(entity);
		const auto &attack = registry.get<AttackComponent>(entity);

		auto &input = registry.get<InputComponent>(entity);
		auto &chase = registry.get<ChaseComponent>(entity);
		auto &movement = registry.get<MovementComponent>(entity);
		auto &render = registry.get<RenderComponent>(entity);

		glm::vec2 enemyScale = {1.0f, 1.0f};
		if (registry.has<TransformComponent>(entity)) {
			enemyScale = registry.get<TransformComponent>(entity).scale;
		}

		chase.reactionDelay.step(std::chrono::duration<float>(deltaTime), false);

		const glm::vec2 enemyCenter = {
		    pos.position.x + (enemyHurtBox.x * enemyScale.x) + (enemyHurtBox.w * enemyScale.x) * 0.5f,
		    pos.position.y + (enemyHurtBox.y * enemyScale.y) + (enemyHurtBox.h * enemyScale.y) * 0.5f};

		if (registry.has<RangedAttackComponent>(entity)) {

			handleRangeAttack(enemyCenter, playerCenter, chase, registry.get<RangedAttackComponent>(entity), input,
			                  render, attack);
			continue;
		}

		if (playerCenter.x < patrol.minBounds.x || playerCenter.x > patrol.maxBounds.x) {
			resetChaseState(input, movement, chase);
			continue;
		}

		const float verticalThreshold =
		    playerGrounded ? chase.verticalDetectionHeight : chase.verticalDetectionHeight * 2.0f;

		if (std::abs(playerCenter.y - enemyCenter.y) > verticalThreshold) {
			resetChaseState(input, movement, chase);
			continue;
		}

		if (AttackSystem::isAttacking(attack)) {
			input.attackPressed = false;
			input.horizontalDirection = 0.0f;
			continue;
		}

		const float xDistance = std::abs(playerCenter.x - enemyCenter.x);
		const float directionToPlayer = playerCenter.x > enemyCenter.x ? 1.0f : -1.0f;

		if (const float stopDistance =
		        (playerHurtBox.w * playerScale.x * 0.5f) + (enemyHurtBox.w * enemyScale.x * 0.5f);
		    xDistance <= stopDistance) {
			input.horizontalDirection = 0.0f;
			render.flipX = directionToPlayer < 0;
			input.attackPressed = chase.reactionDelay.isTimeout();
		} else {
			input.horizontalDirection = directionToPlayer;
			render.flipX = directionToPlayer < 0;
			movement.maxSpeedX = chase.chaseSpeed;
			input.attackPressed = false;
			chase.reactionDelay.reset();
		}
	}
}

} // namespace GameEngine::ECS::Systems::ChaseSystem
