#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChargeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
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
#include "Game/Factories/ProjectileFactory.hpp"

#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "Engine/Utils/Random.hpp"
#include "Engine/Utils/SpatialHash.hpp"

namespace GameEngine::ECS::Systems::ChaseSystem {
namespace {

struct PlayerContext {
	glm::vec2 center = glm::vec2(0.0f);
	SDL_FRect hurtBox = {0, 0, 0, 0};
	glm::vec2 scale = glm::vec2(1.0f, 1.0f);
	bool isGrounded = false;
	const StateComponent *state = nullptr;
};

struct EnemyContext {
	Entity entity;
	InputComponent *input = nullptr;
	ChaseComponent *chase = nullptr;
	MovementComponent *movement = nullptr;
	RenderComponent *render = nullptr;
	AttackComponent *attack = nullptr;
	const PatrolComponent *patrol = nullptr;

	SDL_FRect hitBox;
	SDL_FRect hurtBox;
	glm::vec2 center;
	glm::vec2 scale;
	glm::vec2 position;
};

float getDirectionToPlayer(const EnemyContext &enemy, const PlayerContext &player)
{
	return player.center.x > enemy.center.x ? 1.0f : -1.0f;
}

void resetChargeState(ChargeComponent &charge, const EnemyContext &enemy, const ChaseComponent &chase)
{
	charge.chargeTimer.reset();
	charge.chargePauseTimer.reset();
	enemy.input->attackPressed = false;
	enemy.input->horizontalDirection = 0.0f;
	enemy.movement->maxSpeedX = chase.chaseSpeed;
}

bool updateChargePaused(ChargeComponent &charge, const EnemyContext &enemy, const ChaseComponent &chase,
                        const float deltaTime)
{
	const bool isChargePaused = charge.chargePauseTimer.isRunning() && !charge.chargePauseTimer.isTimeout();
	if (!isChargePaused) {
		return false;
	}

	charge.chargePauseTimer.step(std::chrono::duration<float>(deltaTime), false);

	enemy.input->attackPressed = false;
	enemy.input->horizontalDirection = 0.0f;

	if (charge.chargePauseTimer.isTimeout()) {
		enemy.movement->maxSpeedX = chase.chaseSpeed;
	}
	return true;
}

bool updateCharging(ChargeComponent &charge, const EnemyContext &enemy, const PlayerContext &player,
                    const ChaseComponent &chase, const float deltaTime)
{
	if (const bool isCharging = charge.chargeTimer.isRunning() && !charge.chargeTimer.isTimeout(); !isCharging) {
		return false;
	}

	charge.chargeTimer.step(std::chrono::duration<float>(deltaTime), false);

	const float directionToPlayer = getDirectionToPlayer(enemy, player);
	enemy.input->attackPressed = false;
	enemy.input->horizontalDirection = directionToPlayer;
	enemy.render->setFacingLeft(directionToPlayer < 0);
	enemy.movement->maxSpeedX = chase.chaseSpeed * charge.chargeSpeedMultiplier;

	SDL_FRect enemyHurtBoxRect = Utils::HitBoxUtils::getHitBox(enemy.position, enemy.hurtBox, &enemy.scale);
	SDL_FRect playerHurtBoxRect = Utils::HitBoxUtils::getHitBox(player.center, player.hurtBox, &player.scale);

	const float passThroughDistance = player.hurtBox.w * player.scale.x * charge.chargePassThroughFactor;
	const bool passedThrough = directionToPlayer > 0.0f ? enemy.center.x >= player.center.x + passThroughDistance
	                                                    : enemy.center.x <= player.center.x - passThroughDistance;

	if (SDL_HasRectIntersectionFloat(&enemyHurtBoxRect, &playerHurtBoxRect) && passedThrough) {
		charge.chargeTimer.reset();
		charge.chargePauseTimer.reset();
		charge.chargePauseTimer.step(std::chrono::duration<float>(0.0f), false);
		enemy.input->attackPressed = false;
		enemy.input->horizontalDirection = 0.0f;
		return true;
	}

	if (charge.chargeTimer.isTimeout()) {
		enemy.movement->maxSpeedX = chase.chaseSpeed;
	}
	return true;
}
void resetChaseState(const EnemyContext &enemy)
{
	enemy.input->attackPressed = false;
	enemy.movement->maxSpeedX = enemy.chase->originalPatrolSpeed;
	enemy.chase->reactionDelay.reset();
	enemy.chase->lastKnownPlayerX = std::nullopt;
}

void beginCharge(ChargeComponent &charge, InputComponent &input, ChaseComponent &chase, MovementComponent &movement,
                 RenderComponent &render, const float directionToPlayer)
{
	charge.chargeTimer.reset();
	charge.chargeTimer.step(std::chrono::duration<float>(0.0f), false); // signal it started
	charge.chargeCooldown.reset();
	chase.reactionDelay.reset();
	input.attackPressed = false;
	input.horizontalDirection = directionToPlayer;
	render.setFacingLeft(directionToPlayer < 0);
	movement.maxSpeedX = chase.chaseSpeed * charge.chargeSpeedMultiplier;
}

bool shouldCharge(const ChargeComponent &charge, const ChaseComponent &chase, const float xDistance,
                  const float chargeRange)
{
	return xDistance <= chargeRange && chase.reactionDelay.isTimeout() && charge.chargeCooldown.isTimeout()
	       && Utils::Random::getFloat(0.0f, 1.0f) <= charge.chargeChance;
}

std::optional<PlayerContext> getPlayerContext(Registry &registry)
{
	for (auto view = registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent,
	                               GroundedComponent, StateComponent>();
	     const auto entity : view) {
		const auto &pos = view.get<PositionComponent>(entity).position;
		const auto &hurtBox = view.get<HurtBoxComponent>(entity).hurtBox;
		const auto &transform = view.get<TransformComponent>(entity);
		const auto &ground = view.get<GroundedComponent>(entity);
		const auto &state = view.get<StateComponent>(entity);

		return PlayerContext{Utils::DetectionRangeUtils::calculateCenter(pos, hurtBox, transform.scale), hurtBox,
		                     transform.scale, ground.grounded, &state};
	}
	return std::nullopt;
}
} // namespace

bool checkLineOfSight(const EnemyContext &enemy, const PlayerContext &player, RangedAttackComponent &ranged,
                      const glm::vec2 &projectileSize, const glm::vec2 &hitboxOffset)
{
	const bool facingLeft = (player.center.x - enemy.center.x) < 0.0f;

	glm::vec2 spawnPosition = enemy.position;
	float xOffset = (enemy.hurtBox.x * enemy.scale.x);
	xOffset += facingLeft ? -ranged.projectileOffset.x : ranged.projectileOffset.x;

	spawnPosition.x += xOffset;
	spawnPosition.y += ranged.projectileOffset.y;

	const glm::vec2 rayStart = spawnPosition + hitboxOffset + (projectileSize * 0.5f);

	const glm::vec2 playerTop = player.center - glm::vec2(0.0f, player.hurtBox.h * player.scale.y * 0.25f);

	for (const std::vector targetPoints = {player.center, playerTop}; const auto &targetPoint : targetPoints) {

		if (const bool obstructed = CollisionSystem::isPathObstructed(rayStart, targetPoint, projectileSize);
		    !obstructed) {
			ranged.targetSnapshot = targetPoint;
			return true;
		}
	}
	return false;
}

void handleRangeAttack(const EnemyContext &enemy, const PlayerContext &player, Registry &registry,
                       RangedAttackComponent &ranged)
{

	if (player.state->state == State::Dead) {
		resetChaseState(enemy);
		enemy.attack->attackTimer.reset();
		ranged.targetSnapshot = glm::vec2(0.0f);
		ranged.projectileSpawned = false;
		return;
	}

	const float directionToPlayer = player.center.x > enemy.center.x ? 1.0f : -1.0f;
	const bool withinRange =
	    Utils::DetectionRangeUtils::isWithinDetectionRange(enemy.center, player.center, *enemy.chase);

	if (!withinRange && !AttackSystem::isAttacking(*enemy.attack)) {
		resetChaseState(enemy);
		return;
	}

	if (withinRange) {
		enemy.chase->lastKnownPlayerX = player.center.x;
	}

	float progress = 0.0f;
	if (AttackSystem::isAttacking(*enemy.attack) && enemy.attack->attackTimer.getLength().count() > 0.0f) {
		progress = enemy.attack->attackTimer.getTime().count() / enemy.attack->attackTimer.getLength().count();
	}

	if (progress <= enemy.attack->activeWindUpLimit) {
		glm::vec2 projectileSize = {0.f, 0.f};
		glm::vec2 hitboxOffset = {0.0f, 0.0f};
		if (const auto pc = registry.tryGet<ProjectileComponent>(enemy.entity)) {
			const auto &tilesetPath = pc->get().projectileTilesetPath;
			if (!tilesetPath.empty()) {
				const auto &projectileData = ProjectileFactory::getCachedProjectileData(tilesetPath);
				projectileSize = {projectileData.hitbox.w, projectileData.hitbox.h};
				hitboxOffset = {projectileData.hitbox.x, projectileData.hitbox.y};
			}
		}

		if (!checkLineOfSight(enemy, player, ranged, projectileSize, hitboxOffset)) {
			resetChaseState(enemy);
			enemy.attack->attackTimer.reset();
			return;
		}
	}

	// player is close enough
	enemy.chase->lastKnownPlayerX = player.center.x;
	enemy.input->horizontalDirection = 0.0f;
	enemy.render->setFacingLeft(directionToPlayer < 0);
	if (enemy.chase->reactionDelay.isTimeout()) {
		enemy.input->attackPressed = true;
		enemy.attack->activeWindUpLimit =
		    Utils::Random::getFloat(enemy.attack->windUpLimit * 0.5f, enemy.attack->windUpLimit);
	}
}

void handleMeleeAttack(const EnemyContext &enemy, const PlayerContext &player)
{
	if (player.center.x < enemy.patrol->minBounds.x || player.center.x > enemy.patrol->maxBounds.x) {
		resetChaseState(enemy);
		return;
	}

	const float verticalThreshold = Utils::DetectionRangeUtils::getVerticalThreshold(*enemy.chase, player.isGrounded);

	if (std::abs(player.center.y - enemy.center.y) > verticalThreshold) {
		resetChaseState(enemy);
		return;
	}

	enemy.chase->lastKnownPlayerX = player.center.x;

	if (AttackSystem::isAttacking(*enemy.attack)) {
		float progress = 0.0f;
		if (enemy.attack->attackTimer.getLength().count() > 0.0f) {
			progress = enemy.attack->attackTimer.getTime().count() / enemy.attack->attackTimer.getLength().count();
		}

		if (progress <= enemy.attack->activeWindUpLimit) {
			const float directionToPlayer = player.center.x > enemy.center.x ? 1.0f : -1.0f;
			const bool facingLeft = directionToPlayer < 0;

			enemy.render->setFacingLeft(facingLeft);
		}

		enemy.input->attackPressed = false;
		enemy.input->horizontalDirection = 0.0f;
		return;
	}

	const float xDistance = std::abs(player.center.x - enemy.center.x);
	const float directionToPlayer = player.center.x > enemy.center.x ? 1.0f : -1.0f;

	if (const float stopDistance =
	        (player.hurtBox.w * player.scale.x * 0.5f) + (enemy.hurtBox.w * enemy.scale.x * 0.5f);
	    xDistance <= stopDistance + enemy.hitBox.w * 0.5) {
		enemy.input->horizontalDirection = 0.0f;
		if (enemy.chase->reactionDelay.isTimeout()) {
			enemy.input->attackPressed = true;
			enemy.attack->activeWindUpLimit =
			    Utils::Random::getFloat(enemy.attack->windUpLimit * 0.5f, enemy.attack->windUpLimit);
		}
		if (!enemy.input->attackPressed) {
			const bool facingLeft = directionToPlayer < 0;
			enemy.render->setFacingLeft(facingLeft);
		}
	} else {
		enemy.input->horizontalDirection = directionToPlayer;
		const bool facingLeft = directionToPlayer < 0;
		enemy.render->setFacingLeft(facingLeft);
		enemy.movement->maxSpeedX = enemy.chase->chaseSpeed;
		enemy.input->attackPressed = false;
		enemy.chase->reactionDelay.reset();
	}
}

void update(Registry &registry, const float deltaTime)
{
	PROFILE_SCOPE("ChaseSystem");

	const auto playerCtxOpt = getPlayerContext(registry);

	if (!playerCtxOpt.has_value()) {
		return;
	}
	const PlayerContext &playerCtx = playerCtxOpt.value();

	auto enemyView = registry.view<EnemyTag, InputComponent, PatrolComponent, ChaseComponent, PositionComponent,
	                               HitBoxComponent, HurtBoxComponent, CollisionComponent, MovementComponent,
	                               GroundedComponent, AttackComponent, RenderComponent>();

	for (const auto entity : enemyView) {
		const auto &pos = enemyView.get<PositionComponent>(entity);
		const auto &enemyHurtBox = enemyView.get<HitBoxComponent>(entity).bodyHitBox;
		const auto &enemyHitBox = enemyView.get<HitBoxComponent>(entity).weaponHitBox;

		auto &chase = enemyView.get<ChaseComponent>(entity);

		auto transform = registry.tryGet<TransformComponent>(entity);

		glm::vec2 enemyScale = transform ? transform->get().scale : glm::vec2{1.0f, 1.0f};

		EnemyContext enemyCtx{entity,
		                      &enemyView.get<InputComponent>(entity),
		                      &enemyView.get<ChaseComponent>(entity),
		                      &enemyView.get<MovementComponent>(entity),
		                      &enemyView.get<RenderComponent>(entity),
		                      &enemyView.get<AttackComponent>(entity),
		                      &enemyView.get<PatrolComponent>(entity),
		                      enemyHitBox,
		                      enemyHurtBox,
		                      Utils::DetectionRangeUtils::calculateCenter(pos.position, enemyHurtBox, enemyScale),
		                      enemyScale,
		                      pos.position};

		chase.reactionDelay.step(std::chrono::duration<float>(deltaTime), false);

		if (auto chargeComp = registry.tryGet<ChargeComponent>(entity)) {
			auto &charge = chargeComp->get();
			charge.chargeCooldown.step(std::chrono::duration<float>(deltaTime), false);

			if (playerCtx.state->state == State::Dead) {
				resetChargeState(charge, enemyCtx, chase);
				continue;
			}

			if (updateChargePaused(charge, enemyCtx, chase, deltaTime)) {
				continue;
			}

			if (updateCharging(charge, enemyCtx, playerCtx, chase, deltaTime)) {
				continue;
			}

			if (AttackSystem::isAttacking(*enemyCtx.attack)) {
				continue;
			}

			const float verticalThreshold =
			    Utils::DetectionRangeUtils::getVerticalThreshold(chase, playerCtx.isGrounded);
			if (std::abs(playerCtx.center.y - enemyCtx.center.y) > verticalThreshold) {
				continue;
			}

			if (!Utils::DetectionRangeUtils::isWithinDetectionRange(enemyCtx.center, playerCtx.center, chase)) {
				continue;
			}

			const float xDistance = std::abs(playerCtx.center.x - enemyCtx.center.x);
			const float stopDistance =
			    (playerCtx.hurtBox.w * playerCtx.scale.x * 0.5f) + (enemyCtx.hurtBox.w * enemyCtx.scale.x * 0.5f);

			if (const float chargeRange = chase.detectionWidthRange.value_or(stopDistance * 3.0f);
			    shouldCharge(charge, chase, xDistance, chargeRange)) {
				beginCharge(charge, *enemyCtx.input, chase, *enemyCtx.movement, *enemyCtx.render,
				            getDirectionToPlayer(enemyCtx, playerCtx));
				continue;
			}
		}

		if (auto ranged = registry.tryGet<RangedAttackComponent>(entity)) {
			handleRangeAttack(enemyCtx, playerCtx, registry, ranged->get());
		} else {
			handleMeleeAttack(enemyCtx, playerCtx);
		}
	}
}
} // namespace GameEngine::ECS::Systems::ChaseSystem