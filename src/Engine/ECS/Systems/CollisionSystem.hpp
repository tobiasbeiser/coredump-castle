#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include <SDL3/SDL_rect.h>

namespace GameEngine::ECS::Systems {
struct CollisionSystem {
	static void update(Registry &registry)
	{
		// Get all entities that move (dynamic)
		auto dynamicView =
		    registry.view<PositionComponent, VelocityComponent, CollisionComponent>().exclude<InactiveTag>();
		// Get all static colliders (tiles)
		auto staticView = registry.view<PositionComponent, CollisionComponent>().exclude<InactiveTag>();
		for (const auto dynEntity : dynamicView) {

			auto &[position, previousPosition] = dynamicView.get<PositionComponent>(dynEntity);
			auto &[velocity] = dynamicView.get<VelocityComponent>(dynEntity);
			const auto &[collisionBox] = dynamicView.get<CollisionComponent>(dynEntity);

			SDL_FRect entityBox{position.x + collisionBox.x, position.y + collisionBox.y, collisionBox.w,
			                    collisionBox.h};

			bool grounded = false;

			// --- Iterate over collidable tiles/entities ---
			for (const auto staticEntity : staticView) {
				if (staticEntity == dynEntity)
					continue;

				const auto &[staticPosition, staticPreviousPosition] = staticView.get<PositionComponent>(staticEntity);
				const auto &[staticCollisionBox] = staticView.get<CollisionComponent>(staticEntity);

				SDL_FRect tileBox{staticPosition.x + staticCollisionBox.x, staticPosition.y + staticCollisionBox.y,
				                  staticCollisionBox.w, staticCollisionBox.h};

				resolveAabbCollision(entityBox, tileBox, position, velocity);
				previousPosition = position;

				if (registry.has<GroundedComponent>(dynEntity)) {
					SDL_FRect sensorBox{position.x + collisionBox.x, position.y + collisionBox.y + collisionBox.h,
					                    collisionBox.w, 1};

					SDL_FRect sensorIntersection;
					if (SDL_GetRectIntersectionFloat(&sensorBox, &tileBox, &sensorIntersection)) {
						if (sensorIntersection.w > sensorIntersection.h)
							grounded = true;
					}
					auto &[cGrounded] = registry.get<GroundedComponent>(dynEntity);
					cGrounded = grounded;
				}
			}
		}
	}

  private:
	static void resolveAabbCollision(const SDL_FRect &entityBox, const SDL_FRect &tileBox, glm::vec2 &pos,
	                                 glm::vec2 &vel)
	{
		SDL_FRect intersection{};
		if (SDL_GetRectIntersectionFloat(&entityBox, &tileBox, &intersection)) {
			if (intersection.w < intersection.h) {
				// Horizontal
				if (vel.x > 0) {
					pos.x -= intersection.w;
					vel.x = 0;
				} else if (vel.x < 0) {
					pos.x += intersection.w;
					vel.x = 0;
				}
			} else {
				// Vertical
				if (vel.y > 0) {
					pos.y -= intersection.h;
					vel.y = 0;
				} else if (vel.y < 0) {
					pos.y += intersection.h;
					vel.y = 0;
				}
			}
		}
	}
};

} // namespace GameEngine::ECS::Systems
