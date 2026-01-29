#include "AiDecisionSystem.hpp"

#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/MeleeAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/AiStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/ChaseComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/VisionComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"

#include <SDL3/SDL_log.h>

namespace {
bool canSeeTarget(const SDL_FRect &sourceHitBox, const float detection_range, const SDL_FRect &targetHitBox)
{
	const glm::vec2 sourceCenter = GameEngine::Utils::DetectionRangeUtils::getRectCenter(sourceHitBox);
	const glm::vec2 targetCenter = GameEngine::Utils::DetectionRangeUtils::getRectCenter(targetHitBox);

	const float distance = glm::distance(sourceCenter, targetCenter);
	if (distance > detection_range) {
		return false;
	}

	return !GameEngine::ECS::Systems::CollisionSystem::isPathObstructed(sourceCenter, targetCenter,
	                                                                    glm::vec2{1.0f, 1.0f});
}

bool isInAttackRange(const SDL_FRect &sourceHitBox, const SDL_FRect &targetHitBox, const float attackRange)
{
	const glm::vec2 sourceCenter = GameEngine::Utils::DetectionRangeUtils::getRectCenter(sourceHitBox);
	const glm::vec2 targetCenter = GameEngine::Utils::DetectionRangeUtils::getRectCenter(targetHitBox);

	const float distance = glm::distance(sourceCenter, targetCenter);
	return distance <= attackRange + sourceHitBox.w * 0.5f;
}

} // namespace

void GameEngine::ECS::Systems::AiDecisionSystem::update(Registry &registry)
{
	auto view = registry.view<AiStateComponent, PositionComponent, HitBoxComponent>();

	auto playerView = registry.view<PositionComponent, PlayerTag, HurtBoxComponent>();

	if (playerView.size() != 1) {
		return;
	}
	const auto player = *playerView.begin();
	const auto &[playerPosition, previousPosition] = playerView.get<PositionComponent>(player);
	const auto &[hurtBox] = playerView.get<HurtBoxComponent>(player);
	const auto &playerScaleComponent = registry.tryGet<TransformComponent>(player);

	const auto playerScale = playerScaleComponent ? &playerScaleComponent->get().scale : nullptr;

	const auto playerHitBox = Utils::HitBoxUtils::getHitBox(playerPosition, hurtBox, playerScale);

	for (const auto entity : view) {
		auto &[state] = view.get<AiStateComponent>(entity);
		auto &hitBoxComp = view.get<HitBoxComponent>(entity);
		auto &pos = view.get<PositionComponent>(entity);
		const auto &sourceScaleComponent = registry.tryGet<TransformComponent>(entity);
		const auto sourceScale = sourceScaleComponent ? &sourceScaleComponent->get().scale : nullptr;

		const SDL_FRect sourceHitBox = Utils::HitBoxUtils::getHitBox(pos.position, hitBoxComp.bodyHitBox, sourceScale);

		if (registry.has<VisionComponent>(entity)) {
			const auto &[visionRange] = registry.get<VisionComponent>(entity);

			if (const auto attack = registry.tryGet<AttackComponent>(entity);
			    attack && attack->get().attackTimer.isRunning()) {
				state = AiState::Attack;
				continue;
			}

			if (canSeeTarget(sourceHitBox, visionRange, playerHitBox)) {

				if (const auto melee = registry.tryGet<MeleeAttackComponent>(entity);
				    melee && isInAttackRange(sourceHitBox, playerHitBox, melee->get().attackRange)) {
					state = AiState::Attack;
					continue;
				}
				if (const auto ranged = registry.tryGet<RangedAttackComponent>(entity);
				    ranged && isInAttackRange(sourceHitBox, playerHitBox, ranged->get().attackRange)) {
					state = AiState::Attack;
					continue;
				}
				if (registry.has<ChaseComponent>(entity)) {
					state = AiState::Chase;
					continue;
				}
			}
		}
		if (registry.has<PatrolComponent>(entity)) {
			state = AiState::Patrol;
		} else {
			state = AiState::Idle;
		}
	}
};
