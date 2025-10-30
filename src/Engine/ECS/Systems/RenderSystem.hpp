#pragma once
#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include <algorithm>

namespace GameEngine::ECS::Systems {
class RenderSystem {
  public:
	RenderSystem(Renderer &renderer) : renderer(renderer) {}

	void Update(Registry &registry, const float alpha)
	{
		renderItems.clear();
		// Backgrounds
		auto bgView = registry.view<SpriteComponent, RenderComponent, BackgroundComponent>();
		for (auto entity : bgView) {
			auto &tex = bgView.get<SpriteComponent>(entity);
			auto &render = bgView.get<RenderComponent>(entity);
			auto &bg = bgView.get<BackgroundComponent>(entity);

			std::optional<SDL_FRect> srcOpt;
			std::optional<SDL_FRect> dstOpt;

			const SDL_FRect *src = nullptr;
			SDL_FRect *dst = nullptr;
			if (bg.repeatX) {
				dstOpt = SDL_FRect{.y = bg.yOffset, .h = static_cast<float>(tex.texture->h)};
			}

			renderItems.push_back(RenderItem{.Layer = render.layer,
			                                 .texture = tex.texture,
			                                 .src = srcOpt,
			                                 .dst = dstOpt,
			                                 .parallax = bg.repeatX,
			                                 .scrollFactor = bg.scrollingSpeed,
			                                 .flipX = render.flipX});
		}

		// Tiles, Player
		auto view = registry.view<PositionComponent, SpriteComponent, RenderComponent>();
		for (auto entity : view) {
			auto &pos = view.get<PositionComponent>(entity);
			auto &sprite = view.get<SpriteComponent>(entity);
			auto &render = view.get<RenderComponent>(entity);

			const SDL_FRect src{.x = sprite.TileMapPosition.x,
			                    .y = sprite.TileMapPosition.y,
			                    .w = sprite.TileSize.x,
			                    .h = sprite.TileSize.y};

			// non interpolation

			SDL_FRect dst{.x = pos.position.x, .y = pos.position.y, .w = sprite.TileSize.x, .h = sprite.TileSize.y};
			/*SDL_FRect dst{.x = pos.previousPosition.x * (1 - alpha) + pos.position.x * alpha,
			              .y = pos.previousPosition.y * (1 - alpha) + pos.position.y * alpha,
			              .w = tile.TileSize,
			              .h = tile.TileSize};*/

			renderItems.push_back(RenderItem{.Layer = render.layer,
			                                 .texture = sprite.texture,
			                                 .src = src,
			                                 .dst = dst,
			                                 .parallax = false,
			                                 .flipX = render.flipX});

			if (DebugSystem::Instance().render.showBoundingBoxes && registry.has<CollisionComponent>(entity)) {
				auto col = registry.get<CollisionComponent>(entity).collisionBox;
				col.x = static_cast<int>(pos.position.x + col.x);
				col.y = static_cast<int>(pos.position.y + col.y);

				SDL_Color color =
				    registry.has<MovementComponent>(entity) ? SDL_Color{0, 0, 255, 255} : SDL_Color{255, 0, 0, 255};
				DebugSystem::Instance().debugRects.push_back({col, color});
			}
		}

		std::sort(renderItems.begin(), renderItems.end(),
		          [](const RenderItem &a, const RenderItem &b) { return a.Layer < b.Layer; });

		auto centerCameraView = registry.view<PositionComponent, CenterCameraComponent, SpriteComponent>();

		const int ccv_size = centerCameraView.size();
		if (centerCameraView.size() == 1) {
			const auto &centerCameraEntity = centerCameraView.at(0);
			auto &pos = centerCameraView.get<PositionComponent>(centerCameraEntity);
			auto &tile = centerCameraView.get<SpriteComponent>(centerCameraEntity);

			auto levelView = registry.view<LevelComponent, PositionComponent>();
			for (auto entity : levelView) {
				auto &level = view.get<LevelComponent>(entity);
				auto &posLvl = view.get<PositionComponent>(entity);
				const bool cameraObjInLevelPos =
				    pos.position.x >= posLvl.position.x + tile.TileSize.x / 2 && pos.position.y >= posLvl.position.y;
				const bool cameraObjInLevelSizeX =
				    pos.position.x + tile.TileSize.x / 2 <= posLvl.position.x + level.levelSize.x;
				const bool cameraObjInLevelSizeY = pos.position.y <= posLvl.position.y + level.levelSize.y;

				if (cameraObjInLevelPos && cameraObjInLevelSizeX && cameraObjInLevelSizeY) {
					renderer.CenterCameraOn((pos.position + static_cast<glm::vec2>(tile.TileSize / 2)), posLvl.position,
					                        level.levelSize);
				}
			}

		} else if (centerCameraView.size() > 1) {
			throw std::runtime_error("There cannot be more than on entity with a CenterCameraComponent");
		}

		for (auto &renderItem : renderItems) {
			if (!renderItem.parallax) {
				renderer.RenderTexture(*renderItem.texture, renderItem.src, renderItem.dst, renderItem.flipX);
			} else {
				renderer.RenderTextureTiled(*renderItem.texture, renderItem.dst.value(), renderItem.scrollFactor);
			}
		}
		renderItems.clear();
	}

  private:
	struct RenderItem {
		int Layer;
		SDL_Texture *texture;
		std::optional<SDL_FRect> src;
		std::optional<SDL_FRect> dst;
		bool parallax;
		float scrollFactor = 0;
		bool flipX;
	};
	std::vector<RenderItem> renderItems;

	Renderer &renderer;
};
} // namespace GameEngine::ECS::Systems