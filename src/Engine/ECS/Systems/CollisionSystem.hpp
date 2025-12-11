#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"

#include <Engine/ECS/Components/Tags/PassiveEnemyTag.hpp>
#include <SDL3/SDL_rect.h>

namespace GameEngine::ECS::Systems {
struct CollisionSystem {

	static void resolveHorizontalCollisions(const Entity entity, Registry &registry)
	{
		auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
		auto &[velocity] = registry.get<VelocityComponent>(entity);
		auto &[acceleration] = registry.get<AccelerationComponent>(entity);
		const auto &[collisionBox] = registry.get<CollisionComponent>(entity);

		auto entityScale = glm::vec2(1.0f, 1.0f);
		if (registry.has<TransformComponent>(entity)) {
			entityScale = registry.get<TransformComponent>(entity).scale;
		}

		for (auto staticView = registry.view<PositionComponent, CollisionComponent>().exclude<InactiveTag>();
		     const auto staticEntity : staticView) {
			if (staticEntity == entity)
				continue;

			if (shouldSkipCollision(entity, staticEntity, registry))
				continue;

			const auto &[staticPosition, staticPreviousPosition] = staticView.get<PositionComponent>(staticEntity);
			const auto &[staticCollisionBox] = staticView.get<CollisionComponent>(staticEntity);

			auto staticScale = glm::vec2(1.0f, 1.0f);
			if (registry.has<TransformComponent>(staticEntity)) {
				staticScale = registry.get<TransformComponent>(staticEntity).scale;
			}

			SDL_FRect tileBox{staticPosition.x + staticCollisionBox.x * staticScale.x,
			                  staticPosition.y + staticCollisionBox.y * staticScale.y,
			                  staticCollisionBox.w * staticScale.x, staticCollisionBox.h * staticScale.y};

			resolveHorizontalCollision(collisionBox, tileBox, position, velocity, acceleration, entityScale);
		}
	}

	static void resolveVerticalCollisions(const Entity entity, Registry &registry)
	{
		auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
		auto &[velocity] = registry.get<VelocityComponent>(entity);
		auto &[acceleration] = registry.get<AccelerationComponent>(entity);
		const auto &[collisionBox] = registry.get<CollisionComponent>(entity);

		auto entityScale = glm::vec2(1.0f, 1.0f);
		if (registry.has<TransformComponent>(entity)) {
			entityScale = registry.get<TransformComponent>(entity).scale;
		}

		bool grounded = false;

		for (auto staticView = registry.view<PositionComponent, CollisionComponent>().exclude<InactiveTag>();
		     const auto staticEntity : staticView) {
			if (staticEntity == entity)
				continue;

			if (shouldSkipCollision(entity, staticEntity, registry))
				continue;

			const auto &[staticPosition, staticPreviousPosition] = staticView.get<PositionComponent>(staticEntity);
			const auto &[staticCollisionBox] = staticView.get<CollisionComponent>(staticEntity);

			auto staticScale = glm::vec2(1.0f, 1.0f);
			if (registry.has<TransformComponent>(staticEntity)) {
				staticScale = registry.get<TransformComponent>(staticEntity).scale;
			}

			SDL_FRect tileBox{staticPosition.x + staticCollisionBox.x * staticScale.x,
			                  staticPosition.y + staticCollisionBox.y * staticScale.y,
			                  staticCollisionBox.w * staticScale.x, staticCollisionBox.h * staticScale.y};

			resolveVerticalCollision(collisionBox, tileBox, position, velocity, acceleration, entityScale);

			if (registry.has<GroundedComponent>(entity)) {
				SDL_FRect sensorBox{position.x + collisionBox.x * entityScale.x,
				                    position.y + collisionBox.y * entityScale.y + collisionBox.h * entityScale.y,
				                    collisionBox.w * entityScale.x, 1};

				SDL_FRect sensorIntersection;
				if (SDL_GetRectIntersectionFloat(&sensorBox, &tileBox, &sensorIntersection)) {
					if (sensorIntersection.w > sensorIntersection.h)
						grounded = true;
				}
				auto &[cGrounded] = registry.get<GroundedComponent>(entity);
				cGrounded = grounded;
			}
		}
	}

  private:
	static bool shouldSkipCollision(const Entity entityA, const Entity entityB, Registry &registry)
	{
		const bool aIsPlayer = registry.has<PlayerTag>(entityA);
		const bool bIsPlayer = registry.has<PlayerTag>(entityB);

		const bool aIsBaseEnemy = registry.has<PassiveEnemyTag>(entityA);
		const bool bIsBaseEnemy = registry.has<PassiveEnemyTag>(entityB);

		return (aIsBaseEnemy && bIsPlayer) || (bIsBaseEnemy && aIsPlayer);
	}

	static void resolveHorizontalCollision(const SDL_FRect &collisionA, const SDL_FRect &tileBox, glm::vec2 &pos,
	                                       glm::vec2 &vel, glm::vec2 &acc, const glm::vec2 &scale)
	{
		SDL_FRect intersection{};
		const SDL_FRect entityBox{pos.x + collisionA.x * scale.x, pos.y + collisionA.y * scale.y,
		                          collisionA.w * scale.x, collisionA.h * scale.y};

		if (!SDL_GetRectIntersectionFloat(&entityBox, &tileBox, &intersection))
			return;

		if (intersection.w < intersection.h && intersection.w > 0) {
			const float entityCenterX = entityBox.x + entityBox.w * 0.5f;
			const float tileCenterX = tileBox.x + tileBox.w * 0.5f;
			if (entityCenterX < tileCenterX) {
				pos.x -= intersection.w;
			} else {
				pos.x += intersection.w;
			}
			vel.x = 0;
			acc.x = 0;
		}
	}

	static void resolveVerticalCollision(const SDL_FRect &collisionA, const SDL_FRect &tileBox, glm::vec2 &pos,
	                                     glm::vec2 &vel, glm::vec2 &acc, const glm::vec2 &scale)
	{
		SDL_FRect intersection{};
		const SDL_FRect entityBox{pos.x + collisionA.x * scale.x, pos.y + collisionA.y * scale.y,
		                          collisionA.w * scale.x, collisionA.h * scale.y};

		if (!SDL_GetRectIntersectionFloat(&entityBox, &tileBox, &intersection))
			return;

		if (intersection.w > intersection.h && intersection.h > 0) {
			const float entityCenterY = entityBox.y + entityBox.h * 0.5f;
			const float tileCenterY = tileBox.y + tileBox.h * 0.5f;
			if (entityCenterY < tileCenterY) {
				pos.y -= intersection.h;
			} else {
				pos.y += intersection.h;
			}
			vel.y = 0;
			acc.y = 0;
		}
	}
};

} // namespace GameEngine::ECS::Systems
