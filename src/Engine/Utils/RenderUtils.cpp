
#include "RenderUtils.hpp"

#include "Engine/Debug/ScopedProfiler.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/Graphics/Renderer.hpp"

#include <algorithm>
#include <array>
#include <execution>
#include <optional>
#include <vector>

GameEngine::Utils::RenderUtils::ScopedClip::ScopedClip(SDL_Renderer *renderer, const SDL_Rect &newClip)
    : rendererPtr(renderer)
{
	SDL_GetRenderClipRect(renderer, &oldClip);
	// NOLINTNEXTLINE
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
	    static_cast<int>(std::round(source.x)),
	    static_cast<int>(std::round(source.y)),
	    static_cast<int>(std::round(source.w)),
	    static_cast<int>(std::round(source.h)),
	};
}

void GameEngine::Utils::RenderUtils::setColorMod(RenderItem &renderItem, const SDL_Color colorMod)
{
	renderItem.colorMod = colorMod;
}

glm::vec2 GameEngine::Utils::RenderUtils::getScaledVec(const glm::vec2 vect2D, const float scaling)
{
	return vect2D * scaling;
}

std::optional<GameEngine::Utils::RenderUtils::RenderItem>
GameEngine::Utils::RenderUtils::createSpriteItem(const SpriteRenderContext &ctx, const SDL_FRect &viewport,
                                                 const float globalScaling)
{
	// Tiles, Player
	const SDL_FRect dst = getTileDstFRect({ctx.posComp, ctx.spriteComp, ctx.transformComp}, globalScaling);

	if (!SDL_HasRectIntersectionFloat(&dst, &viewport))
		return std::nullopt;

	const auto &[texture, tileSize, tileMapPosition, spriteColorMod] = ctx.spriteComp;
	const auto &render = ctx.renderComp;
	const double rotation = ctx.transformComp ? ctx.transformComp->get().rotation : 0.0;

	const SDL_FRect src{.x = static_cast<float>(tileMapPosition.x),
	                    .y = static_cast<float>(tileMapPosition.y),
	                    .w = static_cast<float>(tileSize.x),
	                    .h = static_cast<float>(tileSize.y)};

	SDL_Color combinedColorMod = spriteColorMod;
	combinedColorMod.a = (combinedColorMod.a * render.alpha) / 255;

	return RenderItem{.layer = render.layer,
	                  .texture = texture,
	                  .src = src,
	                  .dst = dst,
	                  .parallax = false,
	                  .flipX = render.flipX,
	                  .angle = rotation,
	                  .colorMod = combinedColorMod};
}

SDL_FRect GameEngine::Utils::RenderUtils::getTileDstFRect(const DstRectContext &ctx, const float globalScaling)
{
	const auto &[position, previousPosition] = ctx.posComp;
	const auto &[texture, tileSize, tileMapPosition, spriteColorMod] = ctx.spriteComp;

	const glm::vec2 scale = ctx.transformComp ? ctx.transformComp->get().scale : glm::vec2{1.0f, 1.0f};
	return {.x = position.x * globalScaling,
	        .y = position.y * globalScaling,
	        .w = static_cast<float>(tileSize.x) * scale.x * globalScaling,
	        .h = static_cast<float>(tileSize.y) * scale.y * globalScaling};
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

	const SDL_Texture *lastTexture = nullptr;
	uint32_t lastColorVal = 0;
	bool forceUpdate = true;

	// Outline glow directions for 8-direction offset rendering

	for (auto &renderItem : renderItems) {
		// Render outline glow first (so it appears behind the sprite)
		if (renderItem.hasOutlineGlow && renderItem.dst.has_value() && !renderItem.parallax) {
			// Enable additive blending for a glowing effect
			SDL_SetTextureBlendMode(renderItem.texture, SDL_BLENDMODE_ADD);

			// Render multiple layers at different distances to create soft aura effect
			// Layer 1: Outer glow (larger, more transparent)
			// Layer 2: Middle glow
			// Layer 3: Inner glow (smaller, brighter)
			constexpr int numLayers = 3;

			for (int layer = 0; layer < numLayers; ++layer) {
				constexpr float layerAlphas[numLayers] = {0.3f, 0.5f, 0.8f};
				constexpr float layerScales[numLayers] = {2.0f, 1.5f, 1.0f};
				constexpr std::array<std::pair<float, float>, 8> outlineOffsets = {{{-1.0f, 0.0f},
				                                                                    {1.0f, 0.0f},
				                                                                    {0.0f, -1.0f},
				                                                                    {0.0f, 1.0f},
				                                                                    {-0.7f, -0.7f},
				                                                                    {0.7f, -0.7f},
				                                                                    {-0.7f, 0.7f},
				                                                                    {0.7f, 0.7f}}};
				const float layerThickness = renderItem.outlineThickness * layerScales[layer];
				const auto layerAlpha =
				    static_cast<Uint8>(renderItem.outlineColor.a * renderItem.outlineIntensity * layerAlphas[layer]);

				SDL_SetTextureColorMod(renderItem.texture, renderItem.outlineColor.r, renderItem.outlineColor.g,
				                       renderItem.outlineColor.b);
				SDL_SetTextureAlphaMod(renderItem.texture, layerAlpha);

				// Render multiple offset copies to create glow outline
				for (const auto &[dx, dy] : outlineOffsets) {
					SDL_FRect glowDst = renderItem.dst.value();
					glowDst.x += dx * layerThickness;
					glowDst.y += dy * layerThickness;

					renderer.renderTexture(*renderItem.texture, renderItem.src, glowDst, renderItem.flipX,
					                       renderItem.angle);
				}
			}

			SDL_SetTextureBlendMode(renderItem.texture, SDL_BLENDMODE_BLEND);

			forceUpdate = true;
		}

		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)

		if (const uint32_t currentColorVal = *reinterpret_cast<const uint32_t *>(&renderItem.colorMod);
		    forceUpdate || renderItem.texture != lastTexture || currentColorVal != lastColorVal) {
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
