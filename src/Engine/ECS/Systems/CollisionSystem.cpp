#include "CollisionSystem.hpp"
#include <vector>
#include <algorithm>

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/ProjectileComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/RangedAttackComponent.hpp"
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

	for (auto staticView = registry.view<PositionComponent, CollisionComponent>()
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
	auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
	auto &[velocity] = registry.get<VelocityComponent>(entity);
	auto &[acceleration] = registry.get<AccelerationComponent>(entity);

	auto currentEntityCollisionBox = buildEntityCollisionBox(entity, registry);

	grid.queryExcluding(currentEntityCollisionBox, entity, [&](const SDL_FRect &tileBox) {
		if (resolveHorizontalCollision(tileBox, position, velocity, acceleration, currentEntityCollisionBox)) {
			// We must update the collision box so subsequent checks use the new position
			currentEntityCollisionBox = buildEntityCollisionBox(entity, registry);
		}
	});
}

void resolveVerticalCollisions(const Entity entity, Registry &registry, const Utils::SpatialHash &grid)
{
	auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
	auto &[velocity] = registry.get<VelocityComponent>(entity);
	auto &[acceleration] = registry.get<AccelerationComponent>(entity);

	bool grounded = false;
	auto currentEntityCollisionBox = buildEntityCollisionBox(entity, registry);

	SDL_FRect queryBox = currentEntityCollisionBox;
	// Check for ground tile in the next bucket
	queryBox.h += 1.0f;

	grid.queryExcluding(queryBox, entity, [&](const SDL_FRect &tileBox) {
		if (resolveVerticalCollision(tileBox, position, velocity, acceleration, currentEntityCollisionBox)) {
			// We must update the collision box so subsequent checks use the new position
			currentEntityCollisionBox = buildEntityCollisionBox(entity, registry);
		}

		if (registry.has<GroundedComponent>(entity)) {
			const SDL_FRect sensorBox{currentEntityCollisionBox.x,
			                          currentEntityCollisionBox.y + currentEntityCollisionBox.h,
			                          currentEntityCollisionBox.w, 1};

			SDL_FRect sensorIntersection;
			if (SDL_GetRectIntersectionFloat(&sensorBox, &tileBox, &sensorIntersection)) {
				if (sensorIntersection.w > sensorIntersection.h)
					grounded = true;
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

	for (auto projectileView = registry.view<EnemyTag, ProjectileComponent>().exclude<RangedAttackComponent>();
	     const auto projectileEntity : projectileView) {
		if (!registry.has<PositionComponent>(projectileEntity) || !registry.has<CollisionComponent>(projectileEntity)) {
			continue;
		}

		const auto projectileBox = buildEntityCollisionBox(projectileEntity, registry);
		bool hit = false;

		grid.queryExcluding(projectileBox, projectileEntity, [&](const SDL_FRect &tileBox) {
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
	// Expand query box to cover full projectile path
	const float minX = std::min(start.x, end.x) - projectileSize.x;
	const float maxX = std::max(start.x, end.x) + projectileSize.x;
	const float minY = std::min(start.y, end.y) - projectileSize.y;
	const float maxY = std::max(start.y, end.y) + projectileSize.y;
	
	pathQuery.x = minX;
	pathQuery.y = minY;
	pathQuery.w = maxX - minX;
	pathQuery.h = maxY - minY;

	// Check center line
	std::vector<std::pair<glm::vec2, glm::vec2>> lines;
	lines.push_back({start, end});

	// If thick, check edges
	if (projectileSize.y > 1.0f) {
		const float halfH = projectileSize.y * 0.45f; // slightly less than half to avoid grazing hits
		lines.push_back({{start.x, start.y - halfH}, {end.x, end.y - halfH}});
		lines.push_back({{start.x, start.y + halfH}, {end.x, end.y + halfH}});
	}
	if (projectileSize.x > 1.0f) {
		const float halfW = projectileSize.x * 0.45f;
		lines.push_back({{start.x - halfW, start.y}, {end.x - halfW, end.y}});
		lines.push_back({{start.x + halfW, start.y}, {end.x + halfW, end.y}});
	}

	bool obstructed = false;
	grid.query(pathQuery, [&](const Entity, const SDL_FRect &tileBox) {
		if (obstructed) return;

		for (const auto &line : lines) {
			float x1 = line.first.x, y1 = line.first.y;
			float x2 = line.second.x, y2 = line.second.y;

			if (SDL_GetRectAndLineIntersectionFloat(&tileBox, &x1, &y1, &x2, &y2)) {
				obstructed = true;
				return;
			}
		}
	});

	return obstructed;
}

} // namespace GameEngine::ECS::Systems::CollisionSystem
