#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
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
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/SpatialHash.hpp"

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
	AttackComponent &attack;
	const PatrolComponent &patrol;
	// NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
	SDL_FRect hurtBox = {0, 0, 0, 0};
	glm::vec2 center = glm::vec2(0.0f);
	glm::vec2 scale = glm::vec2(1.0f, 1.0f);
	glm::vec2 position = glm::vec2(0.0f);
};

void resetChaseState(const EnemyContext &enemy)
{
	enemy.input.attackPressed = false;
	enemy.movement.maxSpeedX = enemy.chase.originalPatrolSpeed;
	enemy.chase.reactionDelay.reset();
	enemy.chase.lastKnownPlayerX = std::nullopt;
}

PlayerContext getPlayerContext(Registry &registry)
{

	for (auto view =
	         registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent, GroundedComponent>();
	     const auto entity : view) {
		const auto &pos = registry.get<PositionComponent>(entity).position;
		const auto &hurtBox = registry.get<HurtBoxComponent>(entity).hurtBox;
		const auto &transform = registry.get<TransformComponent>(entity);
		const auto &ground = registry.get<GroundedComponent>(entity);

		const glm::vec2 center = {pos.x + (hurtBox.x * transform.scale.x) + (hurtBox.w * transform.scale.x) * 0.5f,
		                          pos.y + (hurtBox.y * transform.scale.y) + (hurtBox.h * transform.scale.y) * 0.5f};

		return {center, hurtBox, transform.scale, ground.grounded};
	}
	return {};
}

EnemyContext createEnemyContext(Registry &registry, Entity entity)
{
	const auto &pos = registry.get<PositionComponent>(entity);
	const auto &hurtBox = registry.get<HurtBoxComponent>(entity).hurtBox;
    m::vec2 scale =
	    registry.has<TransformComponent>(entity) ? registry.get<TransformComponent>(entity).scale : glm::vec2(1.0f);

	const glm::vec2 center = {pos.position.x + (hurtBox.x * scale.x) + (hurtBox.w * scale.x) * 0.5f,
	                          pos.position.y + (hurtBox.y * scale.y) + (hurtBox.h * scale.y) * 0.5f};

	return {registry.get<InputComponent>(entity),
	        registry.get<ChaseComponent>(entity),
	        registry.get<MovementComponent>(entity),
	        registry.get<RenderComponent>(entity),
	        registry.get<AttackComponent>(entity),
	        registry.get<PatrolComponent>(entity),
	        hurtBox,
	        center,
	        scale,
	        pos.position};
}

void handleRangeAttack(const EnemyContext &enemy, const PlayerContext &player, const Utils::SpatialHash &grid,
                       RangedAttackComponent &ranged, const glm::vec2 &projectileSize)
{
	const float directionToPlayer = player.center.x > enemy.center.x ? 1.0f : -1.0f;
	const float xDistance = std::abs(player.center.x - enemy.center.x);
	const float yDistance = std::abs(player.center.y - enemy.center.y);

	const bool withinRange =
	    (!enemy.chase.detectionWidthRange.has_value() || xDistance <= enemy.chase.detectionWidthRange.value())
	    && yDistance <= enemy.chase.detectionHeightRange;

	if (!withinRange && !AttackSystem::isAttacking(enemy.attack)) {
		resetChaseState(enemy);
		return;
	}

	float progress = 0.0f;
	if (AttackSystem::isAttacking(enemy.attack) && enemy.attack.attackTimer.getLength().count() > 0.0f) {
		progress = enemy.attack.attackTimer.getTime().count() / enemy.attack.attackTimer.getLength().count();
	}

	if (progress <= enemy.attack.windUpLimit) {
		const bool facingLeft = directionToPlayer < 0.0f;
		glm::vec2 spawnPosition = enemy.position;

		if (facingLeft) {
			spawnPosition.x += (enemy.hurtBox.x * enemy.scale.x) - ranged.projectileOffset.x;
		} else {
			spawnPosition.x += (enemy.hurtBox.x * enemy.scale.x) + ranged.projectileOffset.x;
		}
		spawnPosition.y += ranged.projectileOffset.y;

		const glm::vec2 rayStart = spawnPosition + (projectileSize * 0.5f);

		const glm::vec2 playerTop = player.center - glm::vec2(0.0f, player.hurtBox.h * player.scale.y * 0.25f);
		const std::vector targetPoints = {player.center, playerTop};

		bool pathClear = false;
		for (const auto &targetPoint : targetPoints) {
			const bool obstructed = CollisionSystem::isPathObstructed(rayStart, targetPoint, grid, projectileSize);

			if (DebugSystem::instance().render.showRayasts) {
				const SDL_Color color = obstructed ? SDL_Color{255, 0, 0, 255} : SDL_Color{0, 255, 0, 255};
				DebugSystem::instance().debugLines.push_back({rayStart, targetPoint, color});
			}

			if (!obstructed) {
				pathClear = true;
				ranged.targetSnapshot = targetPoint;
				break;
			}
		}

		if (!pathClear) {
			enemy.input.attackPressed = false;
			enemy.chase.reactionDelay.reset();
			enemy.attack.attackTimer.reset();
			return;
		}
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

		auto &attack = registry.get<AttackComponent>(entity);
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

		EnemyContext enemyCtx = {input,  chase,        movement,    render,     attack,
		                         patrol, enemyHurtBox, enemyCenter, enemyScale, pos.position};
		chase.reactionDelay.step(std::chrono::duration<float>(deltaTime), false);

		if (registry.has<RangedAttackComponent>(entity)) {
			auto &ranged = registry.get<RangedAttackComponent>(entity);

			glm::vec2 projectileSize = {0.f, 0.f};
			if (registry.has<ProjectileComponent>(entity)) {
				projectileSize = registry.get<ProjectileComponent>(entity).projectileSize;
			}

			const auto staticColliderGrid = CollisionSystem::buildStaticColliderGrid(registry);
			handleRangeAttack(enemyCtx, playerCtx, staticColliderGrid, ranged, projectileSize);
			continue;
		}
		handleMeleeAttack(enemyCtx, playerCtx);
	}
}

} // namespace GameEngine::ECS::Systems::ChaseSystem
