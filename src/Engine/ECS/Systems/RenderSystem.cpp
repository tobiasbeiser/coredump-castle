#include "RenderSystem.hpp"
#include "../../Debug/DebugSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HealthComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HurtBoxComponent.hpp"
#include "Engine/ECS/Components/Gameplay/MovementComponent.hpp"
#include "Engine/ECS/Components/Physics/CollisionComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/BlinkUtils.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

using namespace GameEngine::ECS::Systems;

RenderSystem::RenderSystem(Renderer &renderer) : renderer(renderer) {}

void RenderSystem::update(Registry &registry, float alpha)
{
	PROFILE_SCOPE("RenderSystem");

	static int renderItemsCapacity = 0;

	renderItems.clear();
	centerCamera(registry);
	auto bgView = registry.view<RenderComponent, BackgroundComponent, PositionComponent>().exclude<InactiveTag>();
	auto view = registry.view<RenderComponent, PositionComponent, SpriteComponent>().exclude<InactiveTag>();

	if (const int requiredCapacity = bgView.size() + view.size(); requiredCapacity > renderItemsCapacity) {
		SDL_Log("Resizing render items");
		renderItemsCapacity = static_cast<int>(requiredCapacity * 1.5f);
		renderItems.reserve(renderItemsCapacity);
	}
	// Backgrounds

	for (const auto entity : bgView) {
		const auto &[layer, flipX] = bgView.get<RenderComponent>(entity);
		const auto &[position, previousPosition] = bgView.get<PositionComponent>(entity);
		const auto &bg = bgView.get<BackgroundComponent>(entity);

		constexpr std::optional<SDL_FRect> srcOpt;
		std::optional<SDL_FRect> dstOpt;

		if (bg.repeatX) {
			dstOpt = SDL_FRect{.x = position.x, .y = bg.yOffset + position.y, .h = static_cast<float>(bg.texture->h)};
		}

		renderItems.emplace_back(RenderItem{.layer = layer,
		                                    .texture = bg.texture,
		                                    .src = srcOpt,
		                                    .dst = dstOpt,
		                                    .parallax = bg.repeatX,
		                                    .scrollFactor = bg.scrollingSpeed,
		                                    .flipX = flipX});
	}

	// Tiles, Player
	for (const auto entity : view) {
		const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
		const auto &[texture, TileSize, TileMapPosition] = view.get<SpriteComponent>(entity);
		const auto &[layer, flipX] = view.get<RenderComponent>(entity);

		glm::vec2 scale = glm::vec2(1.0f, 1.0f);
		if (registry.has<TransformComponent>(entity)) {
			scale = registry.get<TransformComponent>(entity).scale;
		}

		const SDL_FRect src{.x = static_cast<float>(TileMapPosition.x),
		                    .y = static_cast<float>(TileMapPosition.y),
		                    .w = static_cast<float>(TileSize.x),
		                    .h = static_cast<float>(TileSize.y)};

		// non interpolation
		SDL_FRect dst{.x = position.x,
		              .y = position.y,
		              .w = static_cast<float>(TileSize.x) * scale.x,
		              .h = static_cast<float>(TileSize.y) * scale.y};

		// interpolation
		/*SDL_FRect dst{.x = pos.previousPosition.x * (1 - alpha) + pos.position.x * alpha,
		              .y = pos.previousPosition.y * (1 - alpha) + pos.position.y * alpha,
		              .w = tile.TileSize,
		              .h = tile.TileSize};*/

		SDL_Color colorMod = {255, 255, 255, 255};
		if (registry.has<BlinkComponent>(entity) && registry.has<HealthComponent>(entity)) {
			colorMod = Utils::Blink::getBlinkColor(registry, entity);
		}

		renderItems.emplace_back(RenderItem{.layer = layer,
		                                    .texture = texture,
		                                    .src = src,
		                                    .dst = dst,
		                                    .parallax = false,
		                                    .flipX = flipX,
		                                    .colorMod = colorMod});
	}

	std::ranges::sort(renderItems, [](const RenderItem &a, const RenderItem &b) { return a.layer < b.layer; });

	for (auto &renderItem : renderItems) {
		constexpr int rWhite = 255, gWhite = 255, bWhite = 255;
		bool reset = false;
		if (renderItem.colorMod.r != rWhite || renderItem.colorMod.g != gWhite || renderItem.colorMod.b != bWhite) {
			SDL_SetTextureColorMod(renderItem.texture, renderItem.colorMod.r, renderItem.colorMod.g,
			                       renderItem.colorMod.b);
			reset = true;
		}

		if (!renderItem.parallax) {
			renderer.renderTexture(*renderItem.texture, renderItem.src, renderItem.dst, renderItem.flipX);
		} else if (renderItem.dst.has_value()) {
			renderer.renderTextureTiled(*renderItem.texture, renderItem.dst.value(), renderItem.scrollFactor);
		}

		if (reset) {
			SDL_SetTextureColorMod(renderItem.texture, rWhite, gWhite, gWhite);
		}
	}
	renderItems.clear();
}

void RenderSystem::centerCamera(Registry &registry) const
{

	if (auto centerCameraView =
	        registry.view<CenterCameraComponent, PositionComponent, SpriteComponent>().exclude<InactiveTag>();
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
				auto &[layer, flipX] = registry.get<RenderComponent>(levelEntity);
				layer = level.playerLayer;
			}

		} else if (levelView.size() > 1) {
			throw std::runtime_error("Level activation error: More than one active level found!");
		}
	} else if (centerCameraView.size() > 1) {
		throw std::runtime_error("There cannot be more than on entity with a CenterCameraComponent");
	}
}
