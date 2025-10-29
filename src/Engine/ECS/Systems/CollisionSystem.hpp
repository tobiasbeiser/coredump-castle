#pragma once
#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include <SDL3/SDL_rect.h>

namespace GameEngine::ECS::Systems {
struct CollisionSystem {
  public:
	static void Update(Registry &registry)
	{
		std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
		// Get all entities that move (dynamic)
		auto dynamicView = registry.view<PositionComponent, VelocityComponent, CollisionComponent>();
		std::chrono::high_resolution_clock::time_point time1 = std::chrono::high_resolution_clock::now();

		// Get all static colliders (tiles)
		auto staticView = registry.view<PositionComponent, CollisionComponent>();
		std::chrono::high_resolution_clock::time_point time2 = std::chrono::high_resolution_clock::now();

		DebugSystem::Debug(
		    "coll views: "
		    + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time2 - time1).count()) + " us");

		for (auto dynEntity : dynamicView) {
			auto &pos = dynamicView.get<PositionComponent>(dynEntity);
			auto &vel = dynamicView.get<VelocityComponent>(dynEntity);
			auto &col = dynamicView.get<CollisionComponent>(dynEntity);

			SDL_FRect entityBox{pos.position.x + col.collisionBox.x, pos.position.y + col.collisionBox.y,
			                    col.collisionBox.w, col.collisionBox.h};

			bool grounded = false;

			// --- Iterate over collidable tiles/entities ---
			for (auto staticEntity : staticView) {
				// Optionally skip self
				if (staticEntity == dynEntity)
					continue;

				auto &sPos = staticView.get<PositionComponent>(staticEntity);
				auto &sCol = staticView.get<CollisionComponent>(staticEntity);

				SDL_FRect tileBox{sPos.position.x + sCol.collisionBox.x, sPos.position.y + sCol.collisionBox.y,
				                  sCol.collisionBox.w, sCol.collisionBox.h};

				// DebugSystem::Instance().debugRects.push_back({tileBox, SDL_Color{0, 255, 0, 255}});

				ResolveAABBCollision(entityBox, tileBox, pos.position, vel.velocity);
				pos.previousPosition = pos.position;

				if (registry.has<GroundedComponent>(dynEntity)) {
					// Ground check
					SDL_FRect sensorBox{pos.position.x + col.collisionBox.x,
					                    pos.position.y + col.collisionBox.y + col.collisionBox.h, col.collisionBox.w,
					                    1};

					SDL_FRect sensorIntersection;
					if (SDL_GetRectIntersectionFloat(&sensorBox, &tileBox, &sensorIntersection)) {
						if (sensorIntersection.w > sensorIntersection.h)
							grounded = true;
					}
					auto &groundedComp = registry.get<GroundedComponent>(dynEntity);
					groundedComp.grounded = grounded;
				}
			}
		}
		std::chrono::high_resolution_clock::time_point time3 = std::chrono::high_resolution_clock::now();
		DebugSystem::Debug(
		    "coll res: " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(time3 - time2).count())
		    + " us");
	}

  private:
	static void ResolveAABBCollision(const SDL_FRect &entityBox, const SDL_FRect &tileBox, glm::vec2 &pos,
	                                 glm::vec2 &vel)
	{
		SDL_FRect intersection;
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
