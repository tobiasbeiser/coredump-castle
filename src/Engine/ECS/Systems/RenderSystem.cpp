#include "RenderSystem.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/LevelComponent.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Gameplay/Combat/HitBoxComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/BlinkComponent.hpp"
#include "Engine/ECS/Components/Render/CenterCameraComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Components/Tags/InactiveTag.hpp"
#include "Engine/ECS/View.hpp"

#include <algorithm>
#include <execution>
#include <stdexcept>
#include <vector>

using namespace GameEngine::ECS::Systems;

RenderSystem::RenderSystem(Renderer &renderer) : renderer(renderer) {}

auto RenderSystem::getRenderRank(const RenderSystem::RenderItem &item)
{
	return std::tie(item.layer, item.texture, item.colorMod.r, item.colorMod.g, item.colorMod.b, item.colorMod.a,
	                item.flipX);
}

void RenderSystem::update(Registry &registry, float alpha)
{
	{
		PROFILE_SCOPE("RenderSystem::Collect");

		static int renderItemsCapacity = 0;

		renderItems.clear();
		centerCamera(registry);

		auto bgView = registry.view<BackgroundComponent, RenderComponent, PositionComponent>().exclude<InactiveTag>();
		auto view = registry.view<RenderComponent, PositionComponent, SpriteComponent>().exclude<InactiveTag>();

		if (const int requiredCapacity = bgView.size() + view.size(); requiredCapacity > renderItemsCapacity) {
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
				dstOpt =
				    SDL_FRect{.x = position.x, .y = bg.yOffset + position.y, .h = static_cast<float>(bg.texture->h)};
			}

			renderItems.emplace_back(RenderItem{.layer = layer,
			                                    .texture = bg.texture,
			                                    .src = srcOpt,
			                                    .dst = dstOpt,
			                                    .parallax = bg.repeatX,
			                                    .scrollFactor = bg.scrollingSpeed,
			                                    .flipX = flipX});
		}

		const SDL_FRect &viewport = renderer.getViewport();
		// Tiles, Player
		for (const auto entity : view) {
			const auto &[position, previousPosition] = view.get<PositionComponent>(entity);
			const auto &[texture, TileSize, TileMapPosition, spriteColorMod] = view.get<SpriteComponent>(entity);
			const auto &[layer, flipX] = view.get<RenderComponent>(entity);

			const auto transform = registry.tryGet<TransformComponent>(entity);
			const glm::vec2 scale = transform ? transform->get().scale : glm::vec2{1.0f, 1.0f};
			double rotation = transform ? transform->get().rotation : 0.0;

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
			if (SDL_HasRectIntersectionFloat(&dst, &viewport)) {
				renderItems.push_back(RenderItem{.layer = layer,
				                                 .texture = texture,
				                                 .src = src,
				                                 .dst = dst,
				                                 .parallax = false,
				                                 .flipX = flipX,
				                                 .colorMod = spriteColorMod});
			}
			// interpolation
			/*SDL_FRect dst{.x = pos.previousPosition.x * (1 - alpha) + pos.position.x * alpha,
			              .y = pos.previousPosition.y * (1 - alpha) + pos.position.y * alpha,
			              .w = tile.TileSize,
			              .h = tile.TileSize};*/

			if (SDL_HasRectIntersectionFloat(&viewport, &dst)) {
				renderItems.push_back(RenderItem{.layer = layer,
				                                 .texture = texture,
				                                 .src = src,
				                                 .dst = dst,
				                                 .parallax = false,
				                                 .flipX = flipX,
				                                 .angle = rotation,
				                                 .colorMod = spriteColorMod});
			}
		}
		{

			PROFILE_SCOPE("RenderSystem::Sort");

			auto sortStrategy = [](const RenderItem &a, const RenderItem &b) {
				if (a.layer != b.layer) {
					return a.layer < b.layer;
				}

				if (a.texture != b.texture) {
					return a.texture < b.texture;
				}

				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				const uint32_t colorA = *reinterpret_cast<const uint32_t *>(&a.colorMod);
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				const uint32_t colorB = *reinterpret_cast<const uint32_t *>(&b.colorMod);

				if (colorA != colorB) {
					return colorA < colorB;
				}

				return a.flipX < b.flipX;
			};

#if defined(__cpp_lib_execution) && !defined(__APPLE__)
			std::sort(std::execution::seq, renderItems.begin(), renderItems.end(), sortStrategy);
#else
			// apple libc++ does not implement parallel algorithms
			std::ranges::sort(renderItems, sortStrategy);
#endif
		}
		{
			PROFILE_SCOPE("RenderSystem::Submit");

			SDL_Texture *lastTexture = nullptr;
			uint32_t lastColorVal = 0;
			bool forceUpdate = true;

			for (auto &renderItem : renderItems) {
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
				const uint32_t currentColorVal = *reinterpret_cast<const uint32_t *>(&renderItem.colorMod);

				if (forceUpdate || renderItem.texture != lastTexture || currentColorVal != lastColorVal) {
					SDL_SetTextureColorMod(renderItem.texture, renderItem.colorMod.r, renderItem.colorMod.g,
					                       renderItem.colorMod.b);
					SDL_SetTextureAlphaMod(renderItem.texture, renderItem.colorMod.a);

					lastTexture = renderItem.texture;
					lastColorVal = currentColorVal;
					forceUpdate = false;
				}

				if (!renderItem.parallax) {
					renderer.renderTexture(*renderItem.texture, renderItem.src, renderItem.dst, renderItem.flipX);
				} else if (renderItem.dst.has_value()) {
					renderer.renderTextureTiled(*renderItem.texture, renderItem.dst.value(), renderItem.scrollFactor);
				}
			}
			renderItems.clear();
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

			if (const auto render = registry.tryGet<RenderComponent>(levelEntity)) {
				auto &[layer, flipX] = render->get();
				layer = level.playerLayer;
			}

		} else if (levelView.size() > 1) {
			throw std::runtime_error("Level activation error: More than one active level found!");
		}
	} else if (centerCameraView.size() > 1) {
		throw std::runtime_error("There cannot be more than on entity with a CenterCameraComponent");
	}
}
