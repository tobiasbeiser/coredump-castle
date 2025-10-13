#include "CollisionSystem.hpp"
#include "SDL3/SDL.h"
#include "DebugSystem.hpp"

void CollisionSystem::Resolve(std::vector<std::unique_ptr<GameObject>>& objects, float deltaTime) const
{
	std::vector<std::reference_wrapper<IEntity>> entities;
	Level* level = nullptr;

	for (auto& obj : objects)
	{
		if (auto* entity = dynamic_cast<IEntity*>(obj.get())) {
			entities.push_back(std::ref(*entity));
		}
		if (auto* lvl = dynamic_cast<Level*>(obj.get())) {
			level = lvl;
		}
	}

	if (!level) return;

	// Simple AABB collision detection and resolution
	for (auto& entity : entities) {
		bool grounded = false;

		const SDL_FRect entityBox = {
			entity.get().position.x + entity.get().collisionBox.x,
			entity.get().position.y + entity.get().collisionBox.y,
			entity.get().collisionBox.w,
			entity.get().collisionBox.h };


		// Check collision with level tiles
		for (const tson::Layer& layer : level->GetMap()->getLayers()) {
			if (layer.getType() == tson::LayerType::TileLayer) {
				std::map<std::tuple<int, int>, tson::Tile*> tileData = layer.getTileData();
				for (const auto& [pos, tile] : tileData) {
					if (tile && tile->getProp("collidable") && tile->getProp("collidable")->getType() == tson::Type::Boolean && tile->getProp("collidable")->getValue<bool>()) {
						const glm::ivec2 tilePos = glm::ivec2(std::get<0>(pos), std::get<1>(pos));
						const glm::ivec2 tileSize = glm::ivec2(tile->getTileSize().x, tile->getTileSize().y);

						const SDL_FRect tileBox = {
							static_cast<float>(tilePos.x * tileSize.x),
							static_cast<float>(tilePos.y * tileSize.y),
							static_cast<float>(tileSize.x),
							static_cast<float>(tileSize.y) };

						if (DebugSystem::Instance().render.showBoundingBoxes) {
							DebugSystem::Instance().debugRects.push_back({ tileBox, SDL_Color{0, 255, 0, 255} });
						}

						ResolveCollisions(entityBox, tileBox, entity);

						// grounded check
						const SDL_FRect sensorBox{
							.x = entity.get().position.x + entity.get().collisionBox.x,
							.y = entity.get().position.y + entity.get().collisionBox.y + entity.get().collisionBox.h,
							.w = entity.get().collisionBox.w,
							.h = 1
						};

						SDL_FRect sensorIntersection{ 0 };
						if (SDL_GetRectIntersectionFloat(&sensorBox, &tileBox, &sensorIntersection)) {
							if (sensorIntersection.w > sensorIntersection.h) {
								grounded = true;

							}
						}
					}
				}
			}
		}

		entity.get().SetGrounded(grounded);
	}
}

void CollisionSystem::ResolveCollisions(const SDL_FRect& entityBox, const SDL_FRect& tileBox, const std::reference_wrapper<IEntity>& entity) const
{
	// AABB collision check
	SDL_FRect intersection{ 0 };
	if (SDL_GetRectIntersectionFloat(&entityBox, &tileBox, &intersection)) {

		if (intersection.w < intersection.h) {
			//horizontal
			if (entity.get().velocity.x > 0) { // right
				entity.get().position.x -= intersection.w;
				entity.get().velocity.x = 0;
			}
			else if (entity.get().velocity.x < 0) { // left
				entity.get().position.x += intersection.w;
				entity.get().velocity.x = 0;
			}
		}
		else {
			//vertical
			if (entity.get().velocity.y > 0) { // down
				entity.get().position.y -= intersection.h;
				entity.get().velocity.y = 0;
			}
			else if (entity.get().velocity.y < 0) { // up
				entity.get().position.y += intersection.h;;
				entity.get().velocity.y = 0;
			}
		}
	}
}
