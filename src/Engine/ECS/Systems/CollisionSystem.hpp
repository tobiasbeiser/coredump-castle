#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include <SDL3/SDL_rect.h>

namespace GameEngine::ECS::Systems {
struct CollisionSystem {

	static void resolveHorizontalCollisions(const Entity entity, Registry &registry)
	{
		auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
		auto &[velocity] = registry.get<VelocityComponent>(entity);
		auto &[acceleration] = registry.get<AccelerationComponent>(entity);
		const auto &[collisionBox] = registry.get<CollisionComponent>(entity);

		for (auto staticView = registry.view<PositionComponent, CollisionComponent>().exclude<InactiveTag>();
		     const auto staticEntity : staticView) {
			if (staticEntity == entity)
				continue;

			const auto &[staticPosition, staticPreviousPosition] = staticView.get<PositionComponent>(staticEntity);
			const auto &[staticCollisionBox] = staticView.get<CollisionComponent>(staticEntity);

			SDL_FRect tileBox{staticPosition.x + staticCollisionBox.x, staticPosition.y + staticCollisionBox.y,
			                  staticCollisionBox.w, staticCollisionBox.h};

			resolveHorizontalCollision(collisionBox, tileBox, position, velocity, acceleration);
		}
	}

	static void resolveVerticalCollisions(const Entity entity, Registry &registry)
	{
		auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
		auto &[velocity] = registry.get<VelocityComponent>(entity);
		auto &[acceleration] = registry.get<AccelerationComponent>(entity);
		const auto &[collisionBox] = registry.get<CollisionComponent>(entity);

		bool grounded = false;

		for (auto staticView = registry.view<PositionComponent, CollisionComponent>().exclude<InactiveTag>();
		     const auto staticEntity : staticView) {
			if (staticEntity == entity)
				continue;

			const auto &[staticPosition, staticPreviousPosition] = staticView.get<PositionComponent>(staticEntity);
			const auto &[staticCollisionBox] = staticView.get<CollisionComponent>(staticEntity);

			SDL_FRect tileBox{staticPosition.x + staticCollisionBox.x, staticPosition.y + staticCollisionBox.y,
			                  staticCollisionBox.w, staticCollisionBox.h};

			resolveVerticalCollision(collisionBox, tileBox, position, velocity, acceleration);

			if (registry.has<GroundedComponent>(entity)) {
				SDL_FRect sensorBox{position.x + collisionBox.x, position.y + collisionBox.y + collisionBox.h,
				                    collisionBox.w, 1};

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
	static void resolveHorizontalCollision(const SDL_FRect &collisionA, const SDL_FRect &tileBox, glm::vec2 &pos,
	                                       glm::vec2 &vel, glm::vec2 &acc)
	{
		SDL_FRect intersection{};
		const SDL_FRect entityBox{pos.x + collisionA.x, pos.y + collisionA.y, collisionA.w, collisionA.h};

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
	                                     glm::vec2 &vel, glm::vec2 &acc)
	{
		SDL_FRect intersection{};
		const SDL_FRect entityBox{pos.x + collisionA.x, pos.y + collisionA.y, collisionA.w, collisionA.h};

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
