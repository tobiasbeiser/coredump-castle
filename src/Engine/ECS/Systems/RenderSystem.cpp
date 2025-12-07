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
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/View.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace GameEngine::ECS::Systems;

RenderSystem::RenderSystem(Renderer &renderer) : renderer(renderer) {}

void RenderSystem::update(Registry &registry, float alpha)
{
	renderItems.clear();
	centerCamera(registry);

	// Backgrounds

	for (auto bgView = registry.view<RenderComponent, BackgroundComponent, PositionComponent>().exclude<InactiveTag>();
	     const auto entity : bgView) {
		const auto &[layer, flipX, scale] = bgView.get<RenderComponent>(entity);
		const auto &[position, previousPosition] = bgView.get<PositionComponent>(entity);
		const auto &bg = bgView.get<BackgroundComponent>(entity);

		constexpr std::optional<SDL_FRect> srcOpt;
		std::optional<SDL_FRect> dstOpt;

		if (bg.repeatX) {
			dstOpt = SDL_FRect{
			    .x = position.x, .y = bg.yOffset + position.y, .h = static_cast<float>(bg.texture->h)};
		}

		renderItems.push_back(RenderItem{.layer = layer,
		                                 .texture = bg.texture,
		                                 .src = srcOpt,
		                                 .dst = dstOpt,
		                                 .parallax = bg.repeatX,
		                                 .scrollFactor = bg.scrollingSpeed,
		                                 .flipX = flipX});
	}

	// Tiles, Player
	for (auto view = registry.view<PositionComponent, SpriteComponent, RenderComponent>().exclude<InactiveTag>();
	     const auto entity : view) {
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &[texture, TileSize, TileMapPosition] = view.get<SpriteComponent>(entity);
		const auto &[layer, flipX, scale] = view.get<RenderComponent>(entity);

		const SDL_FRect src{.x = static_cast<float>(TileMapPosition.x),
		                    .y = static_cast<float>(TileMapPosition.y),
		                    .w = static_cast<float>(TileSize.x),
		                    .h = static_cast<float>(TileSize.y)};

		// non interpolation
		SDL_FRect dst{.x = position.x,
		              .y = position.y,
		              .w = static_cast<float>(TileSize.x) * scale,
		              .h = static_cast<float>(TileSize.y) * scale};

		// interpolation
		/*SDL_FRect dst{.x = pos.previousPosition.x * (1 - alpha) + pos.position.x * alpha,
		              .y = pos.previousPosition.y * (1 - alpha) + pos.position.y * alpha,
		              .w = tile.TileSize,
		              .h = tile.TileSize};*/

		renderItems.push_back(
		    RenderItem{.layer = layer, .texture = texture, .src = src, .dst = dst, .parallax = false, .flipX = flipX});

		if (DebugSystem::instance().render.showBoundingBoxes && registry.has<CollisionComponent>(entity)) {
			auto col = registry.get<CollisionComponent>(entity).collisionBox;
			col.x = position.x + col.x;
			col.y = position.y + col.y;

			const SDL_Color color =
			    registry.has<MovementComponent>(entity) ? SDL_Color{0, 0, 255, 255} : SDL_Color{255, 0, 0, 255};
			DebugSystem::instance().debugRects.push_back({col, color});
		}
	}

	std::ranges::sort(renderItems, [](const RenderItem &a, const RenderItem &b) { return a.layer < b.layer; });

	for (auto &renderItem : renderItems) {
		if (!renderItem.parallax) {
			renderer.renderTexture(*renderItem.texture, renderItem.src, renderItem.dst, renderItem.flipX);
		} else if (renderItem.dst.has_value()) {
			renderer.renderTextureTiled(*renderItem.texture, renderItem.dst.value(), renderItem.scrollFactor);
		}
	}
	renderItems.clear();
}

void RenderSystem::centerCamera(Registry &registry) const
{

	if (auto centerCameraView =
	        registry.view<PositionComponent, CenterCameraComponent, SpriteComponent>().exclude<InactiveTag>();
	    centerCameraView.size() == 1) {
		const auto &centerCameraEntity = centerCameraView.at(0);
		auto &[position, previousPosition] = centerCameraView.get<PositionComponent>(centerCameraEntity);
		const auto &tile = centerCameraView.get<SpriteComponent>(centerCameraEntity);

		auto levelView = registry.view<LevelComponent, PositionComponent>().exclude<InactiveTag>();

		if (levelView.size() == 1) {
			const auto levelEntity = levelView.at(0);
			const auto &level = levelView.get<LevelComponent>(levelEntity);
			const auto &posLvl = levelView.get<PositionComponent>(levelEntity);
			renderer.centerCameraOn((position + static_cast<glm::vec2>(tile.tileSize / 2)), posLvl.position,
			                        level.levelSize);

			if (registry.has<RenderComponent>(levelEntity)) {
				auto &[layer, flipX, scale] = registry.get<RenderComponent>(levelEntity);
				layer = level.playerLayer;
			}

		} else if (levelView.size() > 1) {
			throw std::runtime_error("Level activation error: More than one active level found!");
		}
	} else if (centerCameraView.size() > 1) {
		throw std::runtime_error("There cannot be more than on entity with a CenterCameraComponent");
	}
}
