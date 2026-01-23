#include "CollisionSystem.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Core/VelocityComponent.hpp"
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

SDL_FRect buildEntityCollisionBox(const PositionComponent &positionComponent, const CollisionComponent &collision,
                                  const TransformComponent *transform)
{
	glm::vec2 scale{1.0f, 1.0f};

	if (transform) {
		scale = transform->scale;
	}

	return SDL_FRect{positionComponent.position.x + collision.collisionBox.x * scale.x,
	                 positionComponent.position.y + collision.collisionBox.y * scale.y,
	                 collision.collisionBox.w * scale.x, collision.collisionBox.h * scale.y};
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

	for (auto view = registry.view<CollisionComponent, PositionComponent>().exclude<InactiveTag, PlayerTag, EnemyTag>();
	     const auto entity : view) {
		const auto &[collisionBox] = view.get<CollisionComponent>(entity);
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);

		glm::vec2 scale{1.0f, 1.0f};
		if (const auto transform = registry.tryGet<TransformComponent>(entity)) {
			scale = transform->get().scale;
		}

		SDL_FRect tileBox{position.x + collisionBox.x * scale.x, position.y + collisionBox.y * scale.y,
		                  collisionBox.w * scale.x, collisionBox.h * scale.y};

		const bool isHazard = registry.has<SpikeComponent>(entity);
		grid.insert(entity, tileBox, isHazard ? Utils::ColliderType::Hazard : Utils::ColliderType::Normal);
	}
	return grid;
}

void resolveHorizontalCollisions(const EntityPhysicsState &state, const Utils::SpatialHash &grid)
{
	auto entityBox = buildEntityCollisionBox(state.pos, state.col, state.transform);

	grid.queryExcluding(entityBox, state.entity,
	                    [&](Entity tileEntity, const SDL_FRect &tileBox, Utils::ColliderType type) {
		                    if (resolveHorizontalCollision(tileBox, state.pos.position, state.vel.velocity,
		                                                   state.acc.acceleration, entityBox)) {
			                    entityBox = buildEntityCollisionBox(state.pos, state.col, state.transform);
		                    }
	                    });
}

void resolveVerticalCollisions(const EntityPhysicsState &state, const Utils::SpatialHash &grid)
{
	auto entityBox = buildEntityCollisionBox(state.pos, state.col, state.transform);

	grid.queryExcluding(entityBox, state.entity, [&](const Entity, const SDL_FRect &tileBox, Utils::ColliderType type) {
		if (resolveVerticalCollision(tileBox, state.pos.position, state.vel.velocity, state.acc.acceleration,
		                             entityBox)) {
			entityBox = buildEntityCollisionBox(state.pos, state.col, state.transform);
		}
	});
}

void updateGroundedState(const EntityPhysicsState &state, GroundedComponent &groundedComp,
                         LastGroundedPositionComponent *lastGroundedComp, const Utils::SpatialHash &grid)
{
	groundedComp.grounded = false;
	const auto entityBox = buildEntityCollisionBox(state.pos, state.col, state.transform);

	SDL_FRect sensorBox = entityBox;
	sensorBox.y += sensorBox.h;
	sensorBox.h = 2.0f;

	grid.queryExcluding(
	    sensorBox, state.entity, [&](const Entity, const SDL_FRect &tileBox, const Utils::ColliderType type) {
		    SDL_FRect intersection;
		    if (SDL_GetRectIntersectionFloat(&sensorBox, &tileBox, &intersection)) {

			    if (intersection.w > intersection.h) {
				    groundedComp.grounded = true;

				    if (lastGroundedComp && type != Utils::ColliderType::Hazard) {
					    constexpr float overlapRatioThreshold = 0.7f;
					    const float overlapRatio = intersection.w / entityBox.w;

					    if (overlapRatio > overlapRatioThreshold) {
						    const float tileCenterX = tileBox.x + (tileBox.w * 0.5f);
						    const float targetX = tileCenterX - (entityBox.w * 0.5f);
						    const float targetY = tileBox.y - entityBox.h;

						    const float offsetX = entityBox.x - state.pos.position.x;
						    const float offsetY = entityBox.y - state.pos.position.y;

						    lastGroundedComp->position = glm::vec2(targetX - offsetX, targetY - offsetY);
					    }
				    }
			    }
		    }
	    });
}

} // namespace GameEngine::ECS::Systems::CollisionSystem