#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
#include "Engine/ECS/Components/Physics/AccelerationComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Physics/GroundedComponent.hpp"
#include "Engine/ECS/Components/Tags/EnemyTag.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/Components/Tags/PlayerTag.hpp"
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

		for (auto staticView = registry.view<PositionComponent, CollisionComponent>()
		                           .exclude<InactiveTag>()
		                           .exclude<PlayerTag>()
		                           .exclude<EnemyTag>();
		     const auto staticEntity : staticView) {
			if (staticEntity == entity)
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

			auto scaledEntityBox = buildEntityCollisionBox(entity, registry);

			resolveHorizontalCollision(tileBox, position, velocity, acceleration, scaledEntityBox);
		}
	}

	static void resolveVerticalCollisions(const Entity entity, Registry &registry)
	{
		auto &[position, previousPosition] = registry.get<PositionComponent>(entity);
		auto &[velocity] = registry.get<VelocityComponent>(entity);
		auto &[acceleration] = registry.get<AccelerationComponent>(entity);

		bool grounded = false;

		for (auto staticView = registry.view<PositionComponent, CollisionComponent>()
		                           .exclude<InactiveTag>()
		                           .exclude<PlayerTag>()
		                           .exclude<EnemyTag>();
		     const auto staticEntity : staticView) {
			if (staticEntity == entity)
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

			auto scaledEntityBox = buildEntityCollisionBox(entity, registry);

			resolveVerticalCollision(tileBox, position, velocity, acceleration, scaledEntityBox);

			if (registry.has<GroundedComponent>(entity)) {
				scaledEntityBox = buildEntityCollisionBox(entity, registry);
				SDL_FRect sensorBox{scaledEntityBox.x, scaledEntityBox.y + scaledEntityBox.h, scaledEntityBox.w, 1};

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
	static SDL_FRect buildEntityCollisionBox(const Entity entity, Registry &registry)
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

	static void resolveHorizontalCollision(const SDL_FRect &tileBox, glm::vec2 &pos, glm::vec2 &vel, glm::vec2 &acc,
	                                       const SDL_FRect scaledEntityBox)
	{
		SDL_FRect intersection{};

		if (!SDL_GetRectIntersectionFloat(&scaledEntityBox, &tileBox, &intersection))
			return;

		if (intersection.w < intersection.h && intersection.w > 0) {
			const float entityCenterX = scaledEntityBox.x + scaledEntityBox.w * 0.5f;
			if (const float tileCenterX = tileBox.x + tileBox.w * 0.5f; entityCenterX < tileCenterX) {
				pos.x -= intersection.w;
			} else {
				pos.x += intersection.w;
			}
			vel.x = 0;
			acc.x = 0;
		}
	}

	static void resolveVerticalCollision(const SDL_FRect &tileBox, glm::vec2 &pos, glm::vec2 &vel, glm::vec2 &acc,
	                                     const SDL_FRect scaledEntityBox)
	{
		SDL_FRect intersection{};

		if (!SDL_GetRectIntersectionFloat(&scaledEntityBox, &tileBox, &intersection))
			return;

		if (intersection.w > intersection.h && intersection.h > 0) {
			const float entityCenterY = scaledEntityBox.y + scaledEntityBox.h * 0.5f;
			if (const float tileCenterY = tileBox.y + tileBox.h * 0.5f; entityCenterY < tileCenterY) {
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
