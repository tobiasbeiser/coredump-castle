#include "CollisionSystem.hpp"

#include "Engine/Debug/DebugSystem.hpp"

#include <algorithm>
#include <vector>

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

#include <SDL3/SDL_pixels.h>

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

void handleEnemyProjectileCollisions(Registry &registry, const Utils::SpatialHash &grid)
{
	constexpr int projectileDamage = 10;

	for (auto projectileView = registry.view<EnemyTag, ProjectileComponent>().exclude<RangedAttackComponent>();
	     const auto projectileEntity : projectileView) {
		if (!registry.has<PositionComponent>(projectileEntity) || !registry.has<CollisionComponent>(projectileEntity)) {
			continue;
		}

		const auto projectileBox = buildEntityCollisionBox(projectileEntity, registry);
		bool hit = false;

		grid.queryExcluding(projectileBox, projectileEntity, [&](const Entity, const SDL_FRect &tileBox) {
			if (SDL_HasRectIntersectionFloat(&projectileBox, &tileBox)) {
				hit = true;
			}
		});

		if (hit) {
			registry.deleteEntity(projectileEntity);
			continue;
		}

		for (auto playerView = registry.view<PlayerTag, CollisionComponent, PositionComponent>();
		     const auto playerEntity : playerView) {
			const auto playerBox = buildEntityCollisionBox(playerEntity, registry);
			if (SDL_HasRectIntersectionFloat(&projectileBox, &playerBox)) {
				if (!registry.has<PendingDamageComponent>(playerEntity)) {
					registry.registerComponent(playerEntity, PendingDamageComponent{projectileDamage});
				}
				registry.deleteEntity(projectileEntity);
				break;
			}
		}
	}
}

bool isPathObstructed(const glm::vec2 &start, const glm::vec2 &end, const Utils::SpatialHash &grid,
                      const glm::vec2 &projectileSize)
{

	SDL_FRect pathQuery;

	const float minX = std::min(start.x, end.x) - projectileSize.x;
	const float maxX = std::max(start.x, end.x) + projectileSize.x;
	const float minY = std::min(start.y, end.y) - projectileSize.y;
	const float maxY = std::max(start.y, end.y) + projectileSize.y;

	pathQuery.x = minX;
	pathQuery.y = minY;
	pathQuery.w = maxX - minX;
	pathQuery.h = maxY - minY;

	std::vector<std::pair<glm::vec2, glm::vec2>> lines;
	lines.push_back({start, end});

	const float halfW = projectileSize.x * 0.5f;
	const float halfH = projectileSize.y * 0.5f;

	lines.push_back({{start.x - halfW, start.y - halfH}, {end.x - halfW, end.y - halfH}}); // Top-Left
	lines.push_back({{start.x + halfW, start.y - halfH}, {end.x + halfW, end.y - halfH}}); // Top-Right
	lines.push_back({{start.x - halfW, start.y + halfH}, {end.x - halfW, end.y + halfH}}); // Bottom-Left
	lines.push_back({{start.x + halfW, start.y + halfH}, {end.x + halfW, end.y + halfH}}); // Bottom-Right

	bool obstructed = false;
	grid.query(pathQuery, [&](const Entity, const SDL_FRect &tileBox) {
		if (obstructed)
			return;

		for (const auto &line : lines) {
			float x1 = line.first.x, y1 = line.first.y;
			float x2 = line.second.x, y2 = line.second.y;

			if (SDL_GetRectAndLineIntersectionFloat(&tileBox, &x1, &y1, &x2, &y2)) {
				obstructed = true;
				return;
			}
		}
	});

	// Done here since it could be demanding to recalculate the path lines inside debugsystem
	const SDL_Color color = obstructed ? SDL_Color{255, 0, 0, 255} : SDL_Color{0, 255, 0, 255};
	DebugSystem::addLine(start, end, color);

	return obstructed;
}

} // namespace GameEngine::ECS::Systems::CollisionSystem
