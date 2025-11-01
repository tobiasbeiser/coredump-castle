#include "RenderSystem.hpp"
#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/View.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace GameEngine::ECS::Systems;

RenderSystem::RenderSystem(Renderer &renderer) : renderer(renderer) {}

void RenderSystem::Update(Registry &registry, float alpha)
{
	renderItems.clear();

	const int currentLevelId = GetCurrentLevelId(registry);
	CenterCamera(registry, currentLevelId);

	// Backgrounds
	for (auto bgView = registry.view<RenderComponent, BackgroundComponent>(); const auto entity : bgView) {
		const auto &[layer, flipX] = bgView.get<RenderComponent>(entity);
		const auto &bg = bgView.get<BackgroundComponent>(entity);

		if (bg.levelId != currentLevelId) {
			continue;
		}

		constexpr std::optional<SDL_FRect> srcOpt;
		std::optional<SDL_FRect> dstOpt;

		if (bg.repeatX) {
			dstOpt = SDL_FRect{.y = bg.yOffset, .h = static_cast<float>(bg.texture->h)};
		}

		renderItems.push_back(RenderItem{.Layer = layer,
		                                 .texture = bg.texture,
		                                 .src = srcOpt,
		                                 .dst = dstOpt,
		                                 .parallax = bg.repeatX,
		                                 .scrollFactor = bg.scrollingSpeed,
		                                 .flipX = flipX});
	}

	// Tiles, Player
	for (auto view = registry.view<PositionComponent, SpriteComponent, RenderComponent>(); const auto entity : view) {
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &[texture, TileSize, TileMapPosition] = view.get<SpriteComponent>(entity);
		const auto &[layer, flipX] = view.get<RenderComponent>(entity);

		const SDL_FRect src{.x = static_cast<float>(TileMapPosition.x),
		                    .y = static_cast<float>(TileMapPosition.y),
		                    .w = static_cast<float>(TileSize.x),
		                    .h = static_cast<float>(TileSize.y)};

		// non interpolation
		SDL_FRect dst{
		    .x = position.x, .y = position.y, .w = static_cast<float>(TileSize.x), .h = static_cast<float>(TileSize.y)};

		// interpolation
		/*SDL_FRect dst{.x = pos.previousPosition.x * (1 - alpha) + pos.position.x * alpha,
		              .y = pos.previousPosition.y * (1 - alpha) + pos.position.y * alpha,
		              .w = tile.TileSize,
		              .h = tile.TileSize};*/

		renderItems.push_back(
		    RenderItem{.Layer = layer, .texture = texture, .src = src, .dst = dst, .parallax = false, .flipX = flipX});

		if (DebugSystem::Instance().render.showBoundingBoxes && registry.has<CollisionComponent>(entity)) {
			auto col = registry.get<CollisionComponent>(entity).collisionBox;
			col.x = position.x + col.x;
			col.y = position.y + col.y;

			const SDL_Color color =
			    registry.has<MovementComponent>(entity) ? SDL_Color{0, 0, 255, 255} : SDL_Color{255, 0, 0, 255};
			DebugSystem::Instance().debugRects.push_back({col, color});
		}
	}

	std::ranges::sort(renderItems, [](const RenderItem &a, const RenderItem &b) { return a.Layer < b.Layer; });

	for (auto &renderItem : renderItems) {
		if (!renderItem.parallax) {
			renderer.RenderTexture(*renderItem.texture, renderItem.src, renderItem.dst, renderItem.flipX);
		} else {
			renderer.RenderTextureTiled(*renderItem.texture, renderItem.dst.value(), renderItem.scrollFactor);
		}
	}
	renderItems.clear();
}

int RenderSystem::GetCurrentLevelId(Registry &registry)
{
	if (auto centerCameraView = registry.view<PositionComponent, CenterCameraComponent, SpriteComponent>();
	    centerCameraView.size() == 1) {
		const auto &centerCameraEntity = centerCameraView.at(0);
		const auto &[position, previousPosition] = centerCameraView.get<PositionComponent>(centerCameraEntity);
		const auto &tile = centerCameraView.get<SpriteComponent>(centerCameraEntity);

		const auto centerCameraPosition = position + static_cast<glm::vec2>(tile.TileSize / 2);
		for (auto levelView = registry.view<LevelComponent, PositionComponent>(); const auto entity : levelView) {
			const auto &level = levelView.get<LevelComponent>(entity);
			auto &posLvl = levelView.get<PositionComponent>(entity);

			const bool cameraObjInLevelPos =
			    centerCameraPosition.x >= posLvl.position.x && centerCameraPosition.y >= posLvl.position.y;
			const bool cameraObjInLevelSizeX = centerCameraPosition.x <= posLvl.position.x + level.levelSize.x;
			const bool cameraObjInLevelSizeY = centerCameraPosition.y <= posLvl.position.y + level.levelSize.y;

			if (cameraObjInLevelPos && cameraObjInLevelSizeX && cameraObjInLevelSizeY) {
				return level.levelId;
			}
		}
	}
	return 0;
}

void RenderSystem::CenterCamera(Registry &registry, const int levelId) const
{

	if (auto centerCameraView = registry.view<PositionComponent, CenterCameraComponent, SpriteComponent>();
	    centerCameraView.size() == 1) {
		const auto &centerCameraEntity = centerCameraView.at(0);
		auto &[position, previousPosition] = centerCameraView.get<PositionComponent>(centerCameraEntity);
		const auto &tile = centerCameraView.get<SpriteComponent>(centerCameraEntity);

		for (auto levelView = registry.view<LevelComponent, PositionComponent>(); const auto entity : levelView) {
			auto &level = levelView.get<LevelComponent>(entity);
			auto &posLvl = levelView.get<PositionComponent>(entity);

			if (level.levelId == levelId) {
				if (registry.has<RenderComponent>(entity)) {
					auto &[layer, flipX] = registry.get<RenderComponent>(entity);
					layer = level.playerLayer;
				}

				renderer.CenterCameraOn((position + static_cast<glm::vec2>(tile.TileSize / 2)), posLvl.position,
				                        level.levelSize);
			}
		}

	} else if (centerCameraView.size() > 1) {
		throw std::runtime_error("There cannot be more than on entity with a CenterCameraComponent");
	}
}
