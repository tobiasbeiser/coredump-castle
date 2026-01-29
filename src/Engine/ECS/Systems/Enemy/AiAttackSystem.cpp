#include "AiAttackSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/AttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Enemy/AiStateComponent.hpp"
#include "Engine/ECS/Components/Gameplay/InputComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/Systems/CollisionSystem.hpp"
#include "Engine/ECS/Systems/Combat/AttackSystem.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/DetectionRangeUtils.hpp"
#include "Engine/Utils/HitBoxUtils.hpp"
#include "Game/Factories/ProjectileFactory.hpp"

#include <SDL3/SDL_log.h>
#include <optional>

namespace {
struct PlayerContext {
	glm::vec2 center;
	SDL_FRect hurtBox;
	glm::vec2 scale;
};

std::optional<PlayerContext> getPlayerContext(GameEngine::ECS::Registry &registry)
{
	auto view = registry.view<PlayerTag, PositionComponent, HurtBoxComponent, TransformComponent>();
	if (view.size() == 0)
		return std::nullopt;

	const auto entity = *view.begin();
	const auto &pos = view.get<PositionComponent>(entity).position;
	const auto &hurtBox = view.get<HurtBoxComponent>(entity).hurtBox;
	const auto &transform = view.get<TransformComponent>(entity);

	return PlayerContext{GameEngine::Utils::DetectionRangeUtils::calculateCenter(pos, hurtBox, transform.scale),
	                     hurtBox, transform.scale};
}
bool hasLineOfSight(const glm::vec2 &start, const PlayerContext &player, const glm::vec2 &projectileSize)
{
	// Check center
	if (!GameEngine::ECS::Systems::CollisionSystem::isPathObstructed(start, player.center, projectileSize)) {
		return true;
	}
	// Check top (headshot) - helpful if player is behind low cover
	const glm::vec2 playerTop = player.center - glm::vec2(0.0f, player.hurtBox.h * player.scale.y * 0.25f);
	if (!GameEngine::ECS::Systems::CollisionSystem::isPathObstructed(start, playerTop, projectileSize)) {
		return true;
	}
	return false;
}

} // namespace

void GameEngine::ECS::Systems::AiAttackSystem::update(Registry &registry, const float deltaTime)
{
	const auto playerCtxOpt = getPlayerContext(registry);
	if (!playerCtxOpt)
		return;
	const auto &player = playerCtxOpt.value();

	auto view = registry.view<AiStateComponent, InputComponent, AttackComponent, MovementComponent, RenderComponent,
	                          PositionComponent, HitBoxComponent>();

	for (const auto entity : view) {
		if (const auto &[state] = view.get<AiStateComponent>(entity); state != AiState::Attack) {
			continue;
		}

		auto &input = view.get<InputComponent>(entity);
		auto &attack = view.get<AttackComponent>(entity);
		auto &[accelerationRate, maxSpeedX] = view.get<MovementComponent>(entity);
		auto &render = view.get<RenderComponent>(entity);

		const auto previousHorizontalDirection = input.horizontalDirection;
		input.horizontalDirection = 0.0f;
		maxSpeedX = 0.0f;

		if (AttackSystem::isAttacking(attack)) {
			input.attackPressed = false;
			input.horizontalDirection = previousHorizontalDirection;
			continue;
		}

		// face player
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &[bodyHitBox] = view.get<HitBoxComponent>(entity);

		const auto hitbox = Utils::HitBoxUtils::getHitBox(position, bodyHitBox, nullptr);
		const auto centerHitBox = Utils::DetectionRangeUtils::getRectCenter(hitbox);
		const bool faceLeft = player.center.x < centerHitBox.x;
		render.setFacingLeft(faceLeft);

		if (auto ranged = registry.tryGet<RangedAttackComponent>(entity)) {
			glm::vec2 projectileSize = {5.0f, 5.0f}; // Default safety
			glm::vec2 hitboxOffset = {0.0f, 0.0f};

			if (const auto pc = registry.tryGet<ProjectileComponent>(entity)) {
				if (!pc->get().projectileTilesetPath.empty()) {
					const auto &data = ProjectileFactory::getCachedProjectileData(pc->get().projectileTilesetPath);
					projectileSize = {data.hitbox.w, data.hitbox.h};
					hitboxOffset = {data.hitbox.x, data.hitbox.y};
				}
			}

			glm::vec2 muzzlePos = position;
			muzzlePos.x += faceLeft ? ranged->get().projectileOffset.x : -ranged->get().projectileOffset.x;
			muzzlePos.y += ranged->get().projectileOffset.y;

			const glm::vec2 rayStart = muzzlePos + hitboxOffset + (projectileSize * 0.5f);

			if (hasLineOfSight(rayStart, player, projectileSize)) {
				ranged->get().targetSnapshot = player.center;
				input.attackPressed = true;
			} else {
				input.attackPressed = false;
			}
		} else {
			input.attackPressed = true;
		}

		input.horizontalDirection = previousHorizontalDirection;
	}
}
