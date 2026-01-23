#include "CollisionSystem.hpp"

#include "Engine/Debug/DebugSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/PendingDamageComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/LastGroundedPositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Hazard/SpikeComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/SpatialHash.hpp"

namespace GameEngine::ECS::Systems::CollisionSystem {

namespace {

SDL_FRect buildEntityCollisionBox(const Entity entity, Registry &registry)
{
	const auto &positionComponent = registry.get<PositionComponent>(entity);
	const auto &[collisionBox] = registry.get<CollisionComponent>(entity);
	glm::vec2 scale{1.0f, 1.0f};
	if (registry.has<TransformComponent>(entity)) {
		scale = registry.get<TransformComponent>(entity).scale;
	}

	return SDL_FRect{positionComponent.position.x + collisionBox.x * scale.x,
	                 positionComponent.position.y + collisionBox.y * scale.y, collisionBox.w * scale.x,
	                 collisionBox.h * scale.y};
}

bool resolveHorizontalCollision(const SDL_FRect &tileBox, glm::vec2 &pos, glm::vec2 &vel, glm::vec2 &acc,
                                const SDL_FRect scaledEntityBox)
{
	SDL_FRect intersection{};

	if (!SDL_GetRectIntersectionFloat(&scaledEntityBox, &tileBox, &intersection))
		return false;

	if (intersection.w < intersection.h && intersection.w > 0) {
		const float entityCenterX = scaledEntityBox.x + scaledEntityBox.w * 0.5f;
		if (const float tileCenterX = tileBox.x + tileBox.w * 0.5f; entityCenterX < tileCenterX) {
			pos.x -= intersection.w;
		} else {
			pos.x += intersection.w;
		}
		vel.x = 0;
		acc.x = 0;
		return true;
	}

	return false;
}

bool resolveVerticalCollision(const SDL_FRect &tileBox, glm::vec2 &pos, glm::vec2 &vel, glm::vec2 &acc,
                              const SDL_FRect scaledEntityBox)
{
	SDL_FRect intersection{};

	if (!SDL_GetRectIntersectionFloat(&scaledEntityBox, &tileBox, &intersection))
		return false;

	if (intersection.w > intersection.h && intersection.h > 0) {
		const float entityCenterY = scaledEntityBox.y + scaledEntityBox.h * 0.5f;
		if (const float tileCenterY = tileBox.y + tileBox.h * 0.5f; entityCenterY < tileCenterY) {
			pos.y -= intersection.h;
		} else {
			pos.y += intersection.h;
		}
		vel.y = 0;
		acc.y = 0;
		return true;
	}

	return false;
}

} // anonymous namespace

Utils::SpatialHash buildStaticColliderGrid(Registry &registry)
{
	Utils::SpatialHash grid;

	for (auto staticView = registry.view<CollisionComponent, PositionComponent>()
	                           .exclude<InactiveTag>()
	                           .exclude<PlayerTag>()
	                           .exclude<EnemyTag>();
	     const auto staticEntity : staticView) {
		const auto &[staticPosition, staticPreviousPosition] = staticView.get<PositionComponent>(staticEntity);
		const auto &[staticCollisionBox] = staticView.get<CollisionComponent>(staticEntity);

		auto staticScale = glm::vec2(1.0f, 1.0f);
		if (registry.has<TransformComponent>(staticEntity)) {
			staticScale = registry.get<TransformComponent>(staticEntity).scale;
		}

		SDL_FRect tileBox{staticPosition.x + staticCollisionBox.x * staticScale.x,
		                  staticPosition.y + staticCollisionBox.y * staticScale.y, staticCollisionBox.w * staticScale.x,
		                  staticCollisionBox.h * staticScale.y};

		grid.insert(staticEntity, tileBox);
	}

	return grid;
}

void resolveHorizontalCollisions(const Entity entity, Registry &registry, const Utils::SpatialHash &grid)
{
	if (!registry.hasAll<PositionComponent, VelocityComponent, AccelerationComponent>(entity)) {
		return;
	}

	auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
	auto &[velocity] = registry.get<VelocityComponent>(entity);
	auto &[acceleration] = registry.get<AccelerationComponent>(entity);

	auto currentEntityCollisionBox = buildEntityCollisionBox(entity, registry);

	grid.queryExcluding(currentEntityCollisionBox, entity, [&](const Entity tileEntity, const SDL_FRect &tileBox) {
		if (resolveHorizontalCollision(tileBox, position, velocity, acceleration, currentEntityCollisionBox)) {
			// We must update the collision box so subsequent checks use the new position
			currentEntityCollisionBox = buildEntityCollisionBox(entity, registry);
		}
	});
}

void resolveVerticalCollisions(const Entity entity, Registry &registry, const Utils::SpatialHash &grid)
{
	if (!registry.hasAll<PositionComponent, VelocityComponent, AccelerationComponent>(entity)) {
		return;
	}

	auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
	auto &[velocity] = registry.get<VelocityComponent>(entity);
	auto &[acceleration] = registry.get<AccelerationComponent>(entity);

	bool grounded = false;
	auto currentEntityCollisionBox = buildEntityCollisionBox(entity, registry);

	SDL_FRect queryBox = currentEntityCollisionBox;
	queryBox.h += 1.0f;

	grid.queryExcluding(queryBox, entity, [&](const Entity tileEntity, const SDL_FRect &tileBox) {
		if (resolveVerticalCollision(tileBox, position, velocity, acceleration, currentEntityCollisionBox)) {
			currentEntityCollisionBox = buildEntityCollisionBox(entity, registry);
		}

		if (registry.has<GroundedComponent>(entity)) {
			const SDL_FRect sensorBox{currentEntityCollisionBox.x,
			                          currentEntityCollisionBox.y + currentEntityCollisionBox.h,
			                          currentEntityCollisionBox.w, 1};

			SDL_FRect sensorIntersection;
			if (SDL_GetRectIntersectionFloat(&sensorBox, &tileBox, &sensorIntersection)) {
				if (sensorIntersection.w > sensorIntersection.h) {
					grounded = true;

					constexpr float overlapRatioThreshold = 0.7f;
					if (const float overlapRatio = sensorIntersection.w / currentEntityCollisionBox.w;
					    overlapRatio > overlapRatioThreshold) {

						// Store grounded position as center of last secure tile
						if (const bool isHazard = registry.has<SpikeComponent>(tileEntity);
						    registry.has<LastGroundedPositionComponent>(entity) && !isHazard) {
							auto &[lastGroundedPosition] = registry.get<LastGroundedPositionComponent>(entity);
							const float offsetX = currentEntityCollisionBox.x - position.x;
							const float offsetY = currentEntityCollisionBox.y - position.y;
							const float tileCenterX = tileBox.x + (tileBox.w * 0.5f);
							const float targetHitboxX = tileCenterX - (currentEntityCollisionBox.w * 0.5f);
							const float targetHitboxY = tileBox.y - currentEntityCollisionBox.h;
							lastGroundedPosition = glm::vec2(targetHitboxX - offsetX, targetHitboxY - offsetY);
						}
					}
				}
			}
		}
	});

	if (registry.has<GroundedComponent>(entity)) {
		auto &[cGrounded] = registry.get<GroundedComponent>(entity);
		cGrounded = grounded;
	}
}

bool isPathObstructed(const glm::vec2 &start, const glm::vec2 &end, const Utils::SpatialHash &grid,
                      const glm::vec2 &projectileSize)
{
	const float halfW = projectileSize.x * 0.5f;
	const float halfH = projectileSize.y * 0.5f;

	const float minX = std::min(start.x, end.x) - halfW;
	const float maxX = std::max(start.x, end.x) + halfW;
	const float minY = std::min(start.y, end.y) - halfH;
	const float maxY = std::max(start.y, end.y) + halfH;

	const SDL_FRect pathQuery = {minX, minY, maxX - minX, maxY - minY};

	bool obstructed = false;
	grid.query(pathQuery, [&](const Entity, const SDL_FRect &tileBox) {
		if (obstructed)
			return;

		// Minkowski Sum
		const SDL_FRect expandedTile = {tileBox.x - halfW, tileBox.y - halfH, tileBox.w + projectileSize.x,
		                                tileBox.h + projectileSize.y};

		float x1 = start.x, y1 = start.y;
		float x2 = end.x, y2 = end.y;

		if (SDL_GetRectAndLineIntersectionFloat(&expandedTile, &x1, &y1, &x2, &y2)) {
			obstructed = true;
		}
	});

	DebugSystem::addRaycast(start, end, obstructed);

	return obstructed;
}

} // namespace GameEngine::ECS::Systems::CollisionSystem
