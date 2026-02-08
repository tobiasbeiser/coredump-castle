#include "RangedActions.hpp"

#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/PatrolComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementSlowComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Render/CameraShakeComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/VignetteComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/Random.hpp"
#include "Game/Factories/ProjectileFactory.hpp"
#include "Engine/ECS/Components/Render/ChargeTelegraphComponent.hpp"

#include <algorithm>
namespace GameEngine::ECS {

namespace {
struct ProjectileInfo {
	glm::vec2 size{0.0f, 0.0f};
	glm::vec2 offset{0.0f, 0.0f};
	float tileWidth = 0.0f;
};

ProjectileInfo getProjectileInfo(const ProjectileComponent *projComp)
{
	if (!projComp || projComp->projectileTilesetPath.empty())
		return {};

	const auto &data = ProjectileFactory::getCachedProjectileData(projComp->projectileTilesetPath);
	return {{data.hitbox.w * data.scale.x, data.hitbox.h * data.scale.y},
	        {data.hitbox.x * data.scale.x, data.hitbox.y * data.scale.y},
	        static_cast<float>(data.tileSize.x)};
}

} // namespace

glm::vec2 calculateRangedRayStart(Registry &registry, Entity entity, const EntityInfo &enemy, const RangedAttackComponent *ranged,
                                  const ProjectileComponent *proj, const bool facingLeft)
{
	glm::vec2 rayStart = enemy.center;
	if (ranged) {
		const auto hurtBoxOpt = registry.tryGet<HurtBoxComponent>(entity);
		SDL_FRect localHurtBox = hurtBoxOpt ? hurtBoxOpt->get().hurtBox : SDL_FRect{0.0f, 0.0f, 0.0f, 0.0f};

		glm::vec2 spawnPos = enemy.position;
		if (facingLeft) {
			spawnPos.x += (localHurtBox.x * enemy.scale.x) - ranged->projectileOffset.x;
		} else {
			spawnPos.x += ((localHurtBox.x + localHurtBox.w) * enemy.scale.x) + ranged->projectileOffset.x;
		}
		spawnPos.y += (localHurtBox.y * enemy.scale.y) + ranged->projectileOffset.y;

		if (proj && !proj->projectileTilesetPath.empty()) {
			const auto &data = ProjectileFactory::getCachedProjectileData(proj->projectileTilesetPath);
			spawnPos.x += (data.hitbox.x * data.scale.x) + (data.hitbox.w * data.scale.x * 0.5f);
			spawnPos.y += (data.hitbox.y * data.scale.y) + (data.hitbox.h * data.scale.y * 0.5f);
		}

		const float minY = enemy.hitBoxRect.y;
		const float maxY = enemy.hitBoxRect.y + enemy.hitBoxRect.h;
		spawnPos.y = std::clamp(spawnPos.y, minY, maxY);

		rayStart = spawnPos;
	}

	return rayStart;
}

namespace {

std::optional<glm::vec2> getLosTarget(Registry &registry, Entity entity, const EntityInfo &enemy, const EntityInfo &player,
                                      const RangedAttackComponent *ranged, const ProjectileComponent *proj,
                                      const bool facingLeft)
{
	auto [size, offset, tileWidth] = getProjectileInfo(proj);

	const glm::vec2 rayStart = calculateRangedRayStart(registry, entity, enemy, ranged, proj, facingLeft);

	return Utils::DetectionRangeUtils::getTargetWithLineOfSight(rayStart, player.center, player.hitBoxRect,
	                                                            player.scale, size);
}

bool hasLosToAttacker(Registry &registry, Entity entity, const EntityInfo &enemy, const EntityInfo &player, const RangedAttackComponent *ranged,
                      const ProjectileComponent *proj)
{
	const bool facingLeft = (player.center.x - enemy.position.x) < 0.0f;
	return getLosTarget(registry, entity, enemy, player, ranged, proj, facingLeft).has_value();
}

} // namespace

namespace Actions {

void AimAtPlayerWithLOSAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return;

	auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	if (!rangedOpt)
		return;

	auto projOpt = registry.tryGet<ProjectileComponent>(entity);
	auto renderOpt = registry.tryGet<RenderComponent>(entity);
	if (!renderOpt)
		return;

	auto player = getPlayerInfo(registry);
	if (!player) {
		rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
		return;
	}

	// Face the player
	const bool facingLeft = (player->center.x - enemy->center.x) < 0.0f;
	renderOpt->get().setFacingLeft(facingLeft);

	if (auto targetOpt = getLosTarget(registry, entity, *enemy, *player, &rangedOpt->get(),
	                                  projOpt ? &projOpt->get() : nullptr, facingLeft)) {
		rangedOpt->get().targetSnapshot = *targetOpt;
	} else {
		rangedOpt->get().targetSnapshot = glm::vec2(0.0f);
	}
}

void TriggerRangedAttackAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	const auto inputOpt = registry.tryGet<InputComponent>(entity);
	const auto attackOpt = registry.tryGet<AttackComponent>(entity);
	const auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);

	if (!inputOpt || !attackOpt || !rangedOpt)
		return;

	auto &input = inputOpt->get();
	auto &attack = attackOpt->get();
	auto &ranged = rangedOpt->get();

	const auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return;

	for (auto playerView = registry.view<PlayerTag, StateComponent>(); const auto playerEntity : playerView) {
		if (playerView.get<StateComponent>(playerEntity).state == State::Dead) {
			input.attackPressed = false;
			attack.attackTimer.reset();
			ranged.targetSnapshot = glm::vec2(0.0f);
			ranged.projectileSpawned = false;
			return;
		}
		break;
	}

	if (ranged.targetSnapshot == glm::vec2(0.0f)) {
		input.attackPressed = false;
		attack.attackTimer.reset();
		return;
	}

	if (!Systems::AttackSystem::isAttacking(attack)) {
		input.attackPressed = true;
		attack.activeWindUpLimit = Utils::Random::getFloat(attack.windUpLimit * 0.5f, attack.windUpLimit);

		// Ensure we face the player when starting the attack
		if (auto player = getPlayerInfo(registry)) {
			if (const auto render = registry.tryGet<RenderComponent>(entity)) {
				const bool facingLeft = (player->center.x - enemy->center.x) < 0.0f;
				render->get().setFacingLeft(facingLeft);
			}
		}
	}
}

void CancelRangedAttackAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	if (const auto input = registry.tryGet<InputComponent>(entity)) {
		input->get().attackPressed = false;
		input->get().jumpPressed = false;
	}
	if (const auto attack = registry.tryGet<AttackComponent>(entity))
		attack->get().attackTimer.reset();
	if (const auto ranged = registry.tryGet<RangedAttackComponent>(entity)) {
		ranged->get().targetSnapshot = glm::vec2(0.0f);
		ranged->get().projectileSpawned = false;
	}
}

void RetreatFromPlayerAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return;

	auto inputOpt = registry.tryGet<InputComponent>(entity);
	auto renderOpt = registry.tryGet<RenderComponent>(entity);
	auto movementOpt = registry.tryGet<MovementComponent>(entity);
	auto stateOpt = registry.tryGet<StateComponent>(entity);

	if (!inputOpt || !renderOpt)
		return;

	const bool playerOnLeft = player->center.x < enemy->center.x;

	renderOpt->get().setFacingLeft(playerOnLeft);

	// Move away from player
	if (playerOnLeft) {
		inputOpt->get().horizontalDirection = 1.0f;
	} else {
		inputOpt->get().horizontalDirection = -1.0f;
	}

	if (movementOpt) {
		movementOpt->get().maxSpeedX = retreatSpeed;
	}

	if (stateOpt) {
		stateOpt->get().state = State::Rolling;
	}
}

void RecordCornerAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	const auto patrolOpt = registry.tryGet<PatrolComponent>(entity);
	const auto renderOpt = registry.tryGet<RenderComponent>(entity);
	const auto posOpt = registry.tryGet<PositionComponent>(entity);

	if (!patrolOpt || !renderOpt || !posOpt) {
		return;
	}

	if (renderOpt->get().isFacingLeft()) {
		if (!patrolOpt->get().maxX.has_value() || posOpt->get().position.x > patrolOpt->get().maxX.value()) {
			patrolOpt->get().maxX = posOpt->get().position.x;
		}
	} else {
		if (!patrolOpt->get().minX.has_value() || posOpt->get().position.x < patrolOpt->get().minX.value()) {
			patrolOpt->get().minX = posOpt->get().position.x;
		}
	}
}

void EscapeThroughPlayerAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return;

	auto inputOpt = registry.tryGet<InputComponent>(entity);
	auto renderOpt = registry.tryGet<RenderComponent>(entity);
	auto movementOpt = registry.tryGet<MovementComponent>(entity);
	auto smOpt = registry.tryGet<StateMachineComponent>(entity);

	if (!inputOpt || !renderOpt)
		return;

	if (smOpt && smOpt->get().timeInState <= deltaTime * 2.0f) {
		hasStarted = false;
	}

	if (!hasStarted) {
		float direction = 0.0f;
		if (auto patrolOpt = registry.tryGet<PatrolComponent>(entity)) {
			const float centerX = patrolOpt->get().patrolCenter.x;
			direction = (centerX > enemy->center.x) ? 1.0f : -1.0f;
		} else {
			const bool playerOnLeft = player->center.x < enemy->center.x;
			direction = playerOnLeft ? -1.0f : 1.0f;
		}

		storedDirection = direction;
		hasStarted = true;

		const bool movingLeft = storedDirection < 0.0f;
		renderOpt->get().setFacingLeft(movingLeft);
	}

	inputOpt->get().horizontalDirection = storedDirection;

	if (movementOpt) {
		movementOpt->get().maxSpeedX = escapeSpeed;
		if (auto velOpt = registry.tryGet<VelocityComponent>(entity)) {
			velOpt->get().velocity.x = storedDirection * escapeSpeed;
		}
	}
}

void ApplyProximitySlowAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto playerInfo = getPlayerInfo(registry);

	if (!enemy || !playerInfo) {
		return;
	}

	const float distance = glm::distance(enemy->center, playerInfo->center);
	if (distance > slowRange) {
		return;
	}

	for (auto playerView = registry.view<PlayerTag, MovementComponent>(); const auto playerEntity : playerView) {
		if (!registry.has<MovementSlowComponent>(playerEntity)) {
			registry.registerComponent(playerEntity, MovementSlowComponent{});
		}

		auto &slow = registry.get<MovementSlowComponent>(playerEntity);
		slow.speedMultiplier = slowMultiplier;
		slow.slowTimer = Timer{std::chrono::duration<float>(slowDuration)};
		slow.slowTimer.reset();
		break;
	}
}

void ApplyConcussionVignetteAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto playerInfo = getPlayerInfo(registry);

	if (!enemy || !playerInfo) {
		return;
	}

	if (const float distance = glm::distance(enemy->center, playerInfo->center); distance > effectRange) {
		return;
	}

	for (auto playerView = registry.view<PlayerTag, PositionComponent>(); const auto playerEntity : playerView) {
		if (!registry.has<VignetteComponent>(playerEntity)) {
			registry.registerComponent(playerEntity, VignetteComponent{});
		}

		auto &vignette = registry.get<VignetteComponent>(playerEntity);
		vignette.duration = Timer{std::chrono::duration<float>(effectDuration)};
		vignette.duration.reset();
		vignette.color = SDL_FColor{1.0f, 1.0f, 1.0f, 1.0f};
		break;
	}

	if (!registry.has<VignetteComponent>(entity)) {
		registry.registerComponent(entity, VignetteComponent{});
	}
	if (auto enemyVignetteOpt = registry.tryGet<VignetteComponent>(entity)) {
		auto &enemyVignette = enemyVignetteOpt->get();
		enemyVignette.duration = Timer{std::chrono::duration<float>(effectDuration * 0.5f)};
		enemyVignette.duration.reset();
		enemyVignette.color = SDL_FColor{1.0f, 1.0f, 1.0f, 0.85f};
	}
}

void ApplyCameraShakeAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	const auto enemy = getEntityInfo(registry, entity);

	if (const auto playerInfo = getPlayerInfo(registry); !enemy || !playerInfo) {
		return;
	}

	for (auto playerView = registry.view<PlayerTag>(); const auto playerEntity : playerView) {
		if (!registry.has<CameraShakeComponent>(playerEntity)) {
			registry.registerComponent(playerEntity, CameraShakeComponent{});
		}

		auto &shake = registry.get<CameraShakeComponent>(playerEntity);
		shake.amplitude = amplitude;
		shake.decay = false;
		shake.duration = Timer{std::chrono::duration<float>(duration)};
		shake.duration.reset();
		shake.offset = {0.0f, 0.0f};
		break;
	}
}

void PhaseShiftAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	if (const auto render = registry.tryGet<RenderComponent>(entity)) {
		render->get().alpha = 128;
	}
}

void RestoreOpacityAction::execute(Registry &registry, const Entity entity, float deltaTime)
{
	if (const auto render = registry.tryGet<RenderComponent>(entity)) {
		render->get().alpha = 255;
	}
}

void StartSpreadTelegraphAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (auto ranged = registry.tryGet<RangedAttackComponent>(entity)) {
		ranged->get().spreadTelegraphTimer.reset();
		ranged->get().spreadTelegraphTimer.step(std::chrono::duration<float>(0.0f), false);

		if (!registry.has<ChargeTelegraphComponent>(entity)) {
			registry.registerComponent(entity, ChargeTelegraphComponent{});
		}
		auto &telegraph = registry.get<ChargeTelegraphComponent>(entity);
		telegraph.glowColor = SDL_Color{90, 200, 255, 255};
		telegraph.duration = ranged->get().spreadTelegraphTimer.getLength().count() + 0.1f;
		telegraph.pulseFrequency = 0.08f;
		telegraph.outlineThickness = 4.0f;
		telegraph.useOutlineGlow = true;
		telegraph.start();
	}
}

void StopSpreadTelegraphAction::execute(Registry &registry, Entity entity, float deltaTime)
{
	if (auto ranged = registry.tryGet<RangedAttackComponent>(entity)) {
		ranged->get().spreadTelegraphTimer.reset();
	}
	if (auto telegraph = registry.tryGet<ChargeTelegraphComponent>(entity)) {
		telegraph->get().active = false;
	}
}
} // namespace Actions



// Transitions

namespace Transitions {

bool LineOfSightTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);
	if (!enemy || !player)
		return false;

	const auto ranged = registry.tryGet<RangedAttackComponent>(entity);
	const auto proj = registry.tryGet<ProjectileComponent>(entity);

	return hasLosToAttacker(registry, entity, *enemy, *player, ranged ? &ranged->get() : nullptr,
	                        proj ? &proj->get() : nullptr);
}

bool LosLostTransition::canTransition(Registry &registry, const Entity entity, const float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	if (!enemy)
		return false;

	auto player = getPlayerInfo(registry);
	if (!player)
		return true;

	const auto rangedOpt = registry.tryGet<RangedAttackComponent>(entity);
	const auto projOpt = registry.tryGet<ProjectileComponent>(entity);

	if (hasLosToAttacker(registry, entity, *enemy, *player, rangedOpt ? &rangedOpt->get() : nullptr,
	                     projOpt ? &projOpt->get() : nullptr)) {
		losLostDuration = 0.0f;
		return false;
	}

	losLostDuration += deltaTime;
	return losLostDuration >= gracePeriod;
}

bool PlayerTooCloseTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return false;

	const float distance = glm::distance(enemy->center, player->center);
	return distance < closeRange;
}

bool IsGroundedTransition::canTransition(Registry &registry, const Entity entity, float deltaTime) const
{
	auto groundedOpt = registry.tryGet<GroundedComponent>(entity);
	if (!groundedOpt)
		return true;

	return groundedOpt->get().grounded;
}

bool LandedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	const auto sm = registry.tryGet<StateMachineComponent>(entity);
	const auto grounded = registry.tryGet<GroundedComponent>(entity);

	if (!sm || !grounded)
		return false;
	if (sm->get().timeInState < minAirTime)
		return false;

	return grounded->get().grounded;
}

bool StuckTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	const auto sm = registry.tryGet<StateMachineComponent>(entity);
	const auto vel = registry.tryGet<VelocityComponent>(entity);
	const auto input = registry.tryGet<InputComponent>(entity);

	if (!sm || !vel)
		return false;

	if (sm->get().timeInState < 0.2f) {
		currentStuckTime = 0.0f;
		return false;
	}

	if (input && std::abs(input->get().horizontalDirection) > 0.1f) {
		if (input->get().horizontalDirection * vel->get().velocity.x < -1.0f) {
			currentStuckTime = 0.0f;
			return false;
		}
	}

	if (std::abs(vel->get().velocity.x) < threshold) {
		currentStuckTime += deltaTime;
		if (currentStuckTime > requiredTime) {
			return true;
		}
	} else {
		currentStuckTime = 0.0f;
	}

	return false;
}

bool CanRetreatTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);
	const auto patrolOpt = registry.tryGet<PatrolComponent>(entity);

	if (!enemy || !player || !patrolOpt)
		return true;

	const bool playerOnLeft = player->center.x < enemy->center.x;
	const float retreatDir = playerOnLeft ? 1.0f : -1.0f;

	if (retreatDir < 0.0f) {
		if (patrolOpt->get().minX.has_value()) {
			if (enemy->position.x <= patrolOpt->get().minX.value() + 10.0f) {
				return false;
			}
		}
	} else {
		if (patrolOpt->get().maxX.has_value()) {
			if (enemy->position.x >= patrolOpt->get().maxX.value() - 10.0f) {
				return false;
			}
		}
	}

	if (patrolOpt->get().minX.has_value() && player->position.x <= patrolOpt->get().minX.value() + 64.0f) {
		return false;
	}
	if (patrolOpt->get().maxX.has_value() && player->position.x >= patrolOpt->get().maxX.value() - 64.0f) {
		return false;
	}

	return true;
}

bool SafePassedPlayerTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	const auto enemy = getEntityInfo(registry, entity);
	const auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return true;

	bool passed = false;

	if (auto patrolOpt = registry.tryGet<PatrolComponent>(entity)) {
		float direction = 0.0f;
		const float centerX = patrolOpt->get().patrolCenter.x;
		direction = (centerX > player->center.x) ? 1.0f : -1.0f;

		if (direction > 0.0f) {
			passed = enemy->center.x > player->center.x + safetyDistance;
		} else {
			passed = enemy->center.x < player->center.x - safetyDistance;
		}

		if (!passed) {
			if (direction > 0.0f && patrolOpt->get().maxX.has_value()) {
				if (enemy->position.x >= patrolOpt->get().maxX.value() - 10.0f) {
					passed = true;
				}
			} else if (direction < 0.0f && patrolOpt->get().minX.has_value()) {
				if (enemy->position.x <= patrolOpt->get().minX.value() + 10.0f) {
					passed = true;
				}
			}
		}
	} else {
		passed = std::abs(enemy->center.x - player->center.x) > safetyDistance + 50.0f;
	}

	return passed;
}

bool SpreadAttackReadyTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto ranged = registry.tryGet<RangedAttackComponent>(entity);
	if (!ranged)
		return false;

	if (!ranged->get().spreadCooldownTimer.isTimeout()) {
		return false;
	}

	auto enemy = getEntityInfo(registry, entity);
	auto player = getPlayerInfo(registry);

	if (!enemy || !player)
		return false;

	// Check min range
	const float dist = glm::distance(enemy->center, player->center);
	if (dist < ranged->get().spreadMinRange) {
		return false;
	}

	return true;
}

bool SpreadTelegraphFinishedTransition::canTransition(Registry &registry, Entity entity, float deltaTime) const
{
	auto ranged = registry.tryGet<RangedAttackComponent>(entity);
	if (!ranged) {
		return true;
	}

	return ranged->get().spreadTelegraphTimer.isTimeout();
}

} // namespace Transitions

} // namespace GameEngine::ECS
