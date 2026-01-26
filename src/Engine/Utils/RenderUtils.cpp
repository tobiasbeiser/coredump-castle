
#include "RenderUtils.hpp"

#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Debug/ScopedProfiler.hpp"

#include <algorithm>
#include <execution>
#include <stdexcept>
#include <vector>
#include <optional>

GameEngine::Utils::RenderUtils::ScopedClip::ScopedClip(SDL_Renderer *renderer, const SDL_Rect &newClip)
    : rendererPtr(renderer)
{
	SDL_GetRenderClipRect(renderer, &oldClip);
	wasClipping = !SDL_RectEmpty(&oldClip);
	SDL_SetRenderClipRect(renderer, &newClip);
}

GameEngine::Utils::RenderUtils::ScopedClip::~ScopedClip()
{
	if (wasClipping) {
		SDL_SetRenderClipRect(rendererPtr, &oldClip);
	} else {
		SDL_SetRenderClipRect(rendererPtr, nullptr);
	}
}

SDL_Rect GameEngine::Utils::RenderUtils::rectFromFRect(const SDL_FRect &source)
{
	return SDL_Rect{
	    static_cast<int>(source.x),
	    static_cast<int>(source.y),
	    static_cast<int>(source.w),
	    static_cast<int>(source.h),
	};
}

void GameEngine::Utils::RenderUtils::setColorMod(RenderItem &renderItem, const SDL_Color colorMod)
{
	renderItem.colorMod = colorMod;
}

void GameEngine::Utils::RenderUtils::applyScalingFactor(RenderItem &renderItem, const float scalingFactor)
{
	if (renderItem.dst) {
		renderItem.dst->x *= scalingFactor;
		renderItem.dst->y *= scalingFactor;
		renderItem.dst->w *= scalingFactor;
		renderItem.dst->h *= scalingFactor;
	}
}

glm::vec2 GameEngine::Utils::RenderUtils::getScaledVec(const glm::vec2 vect2D, const float scalingFactor)
{
	return vect2D * scalingFactor;
}

std::optional<GameEngine::Utils::RenderUtils::RenderItem>
GameEngine::Utils::RenderUtils::createSpriteItem(const SpriteRenderContext &ctx, const SDL_FRect &viewport,
                                                 const float globalScaling)
{
	// Tiles, Player
	const auto &[position, previousPosition] = ctx.posComp;
	const auto &[texture, TileSize, TileMapPosition, spriteColorMod] = ctx.spriteComp;
	const auto &render = ctx.renderComp;

	const glm::vec2 scale = ctx.transformComp ? ctx.transformComp->get().scale : glm::vec2{1.0f, 1.0f};
	const double rotation = ctx.transformComp ? ctx.transformComp->get().rotation : 0.0;

	const SDL_FRect src{.x = static_cast<float>(TileMapPosition.x),
	                    .y = static_cast<float>(TileMapPosition.y),
	                    .w = static_cast<float>(TileSize.x),
	                    .h = static_cast<float>(TileSize.y)};

	// non interpolation
	SDL_FRect dst{.x = position.x * globalScaling,
	              .y = position.y * globalScaling,
	              .w = static_cast<float>(TileSize.x) * scale.x * globalScaling,
	              .h = static_cast<float>(TileSize.y) * scale.y * globalScaling};
	if (!SDL_HasRectIntersectionFloat(&dst, &viewport))
		return std::nullopt;

	return RenderItem{.layer = render.layer,
	                  .texture = texture,
	                  .src = src,
	                  .dst = dst,
	                  .parallax = false,
	                  .flipX = render.flipX,
	                  .angle = rotation,
	                  .colorMod = spriteColorMod};
}

void GameEngine::Utils::RenderUtils::sortRenderItems(std::vector<RenderItem> &renderItems)
{
	PROFILE_SCOPE("RenderUtils::SortRenderItems");

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

void GameEngine::Utils::RenderUtils::submitRenderItems(std::vector<RenderItem> &renderItems, const Renderer &renderer)
{
	PROFILE_SCOPE("RenderUtils::SubmitRenderItems");

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
			renderer.renderTexture(*renderItem.texture, renderItem.src, renderItem.dst, renderItem.flipX,
			                       renderItem.angle);
		} else if (renderItem.dst.has_value()) {
			renderer.renderTextureTiled(*renderItem.texture, renderItem.dst.value(), renderItem.scrollFactor);
		}
	}
}
