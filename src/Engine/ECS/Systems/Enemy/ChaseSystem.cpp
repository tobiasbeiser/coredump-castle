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

struct PlayerContext {
	glm::vec2 center = glm::vec2(0.0f);
	SDL_FRect hurtBox = {0, 0, 0, 0};
	glm::vec2 scale = glm::vec2(1.0f, 1.0f);
	bool isGrounded = false;
};
// this struct will never be stored or copied, therefore the ignores
struct EnemyContext {
	// NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
	InputComponent &input;
	ChaseComponent &chase;
	MovementComponent &movement;
	RenderComponent &render;
	const AttackComponent &attack;
	const PatrolComponent &patrol;
	// NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
	SDL_FRect hurtBox = {0, 0, 0, 0};
	glm::vec2 center = glm::vec2(0.0f);
	glm::vec2 scale = glm::vec2(1.0f, 1.0f);
};

void resetChaseState(const EnemyContext &enemy)
{
	enemy.input.attackPressed = false;
	enemy.movement.maxSpeedX = enemy.chase.originalPatrolSpeed;
	enemy.chase.reactionDelay.reset();
}

void handleRangeAttack(const EnemyContext &enemy, const PlayerContext &player)
{
	float distX = std::abs(player.center.x - enemy.center.x);
	float distY = std::abs(player.center.y - enemy.center.y);
	const float directionToPlayer = player.center.x > enemy.center.x ? 1.0f : -1.0f;

	if (!AttackSystem::isAttacking(enemy.attack)
	    && (distX > enemy.chase.detectionWidthRange || distY > enemy.chase.detectionHeightRange)) {
		enemy.input.attackPressed = false;
		enemy.chase.reactionDelay.reset();
		return;
	}

	// player is close enough
	enemy.chase.lastKnownPlayerX = player.center.x;
	enemy.input.horizontalDirection = 0.0f;
	enemy.render.flipX = directionToPlayer < 0;
	enemy.input.attackPressed = enemy.chase.reactionDelay.isTimeout();
}

void handleMeleeAttack(const EnemyContext &enemy, const PlayerContext &player)
{
	if (player.center.x < enemy.patrol.minBounds.x || player.center.x > enemy.patrol.maxBounds.x) {
		resetChaseState(enemy);
		return;
	}

	const float verticalThreshold =
	    player.isGrounded ? enemy.chase.detectionHeightRange : enemy.chase.detectionHeightRange * 2.0f;

	if (std::abs(player.center.y - enemy.center.y) > verticalThreshold) {
		resetChaseState(enemy);
		return;
	}

	if (AttackSystem::isAttacking(enemy.attack)) {
		enemy.input.attackPressed = false;
		enemy.input.horizontalDirection = 0.0f;
		return;
	}

	const float xDistance = std::abs(player.center.x - enemy.center.x);
	const float directionToPlayer = player.center.x > enemy.center.x ? 1.0f : -1.0f;

	if (const float stopDistance =
	        (player.hurtBox.w * player.scale.x * 0.5f) + (enemy.hurtBox.w * enemy.scale.x * 0.5f);
	    xDistance <= stopDistance) {
		enemy.input.horizontalDirection = 0.0f;
		enemy.render.flipX = directionToPlayer < 0;
		enemy.input.attackPressed = enemy.chase.reactionDelay.isTimeout();
	} else {
		enemy.input.horizontalDirection = directionToPlayer;
		enemy.render.flipX = directionToPlayer < 0;
		enemy.movement.maxSpeedX = enemy.chase.chaseSpeed;
		enemy.input.attackPressed = false;
		enemy.chase.reactionDelay.reset();
	}
}

} // namespace

void update(Registry &registry, const float deltaTime)
{
	auto playerView =
	    registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent, GroundedComponent>();

	PlayerContext playerCtx;

	for (const auto entity : playerView) {
		const auto &pos = registry.get<PositionComponent>(entity).position;
		playerCtx.hurtBox = registry.get<HurtBoxComponent>(entity).hurtBox;
		playerCtx.scale = registry.get<TransformComponent>(entity).scale;
		playerCtx.isGrounded = registry.get<GroundedComponent>(entity).grounded;

		playerCtx.center = glm::vec2(
		    pos.x + (playerCtx.hurtBox.x * playerCtx.scale.x) + (playerCtx.hurtBox.w * playerCtx.scale.x) * 0.5f,
		    pos.y + (playerCtx.hurtBox.y * playerCtx.scale.y) + (playerCtx.hurtBox.h * playerCtx.scale.y) * 0.5f);
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

		const glm::vec2 enemyCenter = {
		    pos.position.x + (enemyHurtBox.x * enemyScale.x) + (enemyHurtBox.w * enemyScale.x) * 0.5f,
		    pos.position.y + (enemyHurtBox.y * enemyScale.y) + (enemyHurtBox.h * enemyScale.y) * 0.5f};

		EnemyContext enemyCtx = {input, chase, movement, render, attack, patrol, enemyHurtBox, enemyCenter, enemyScale};
		chase.reactionDelay.step(std::chrono::duration<float>(deltaTime), false);

		if (registry.has<RangedAttackComponent>(entity)) {
			handleRangeAttack(enemyCtx, playerCtx);
			continue;
		}
		handleMeleeAttack(enemyCtx, playerCtx);
	}
}

} // namespace GameEngine::ECS::Systems::ChaseSystem
