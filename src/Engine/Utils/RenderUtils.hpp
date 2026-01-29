#pragma once
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/Graphics/Renderer.hpp"

#include <SDL3/SDL_rect.h>
#include <optional>
#include <vector>

namespace GameEngine::Utils::RenderUtils {
// NOLINTBEGIN
struct BackgroundRenderContext {
	const BackgroundComponent &bgComp;
	const PositionComponent &posComp;
	const RenderComponent &renderComp;
};

struct SpriteRenderContext {
	const PositionComponent &posComp;
	const SpriteComponent &spriteComp;
	const RenderComponent &renderComp;
	const std::optional<std::reference_wrapper<TransformComponent>> transformComp;
};

struct DstRectContext {
	const PositionComponent &posComp;
	const SpriteComponent &spriteComp;
	const std::optional<std::reference_wrapper<TransformComponent>> transformComp;
};
// NOLINTEND

struct RenderItem {
	int layer = 0;
	SDL_Texture *texture = nullptr;
	std::optional<SDL_FRect> src{};
	std::optional<SDL_FRect> dst{};
	bool parallax = false;
	float scrollFactor = 0;
	bool flipX = false;
	double angle = 0.0;
	SDL_Color colorMod = {255, 255, 255, 255};
};

class ScopedClip {
  public:
	ScopedClip(SDL_Renderer *renderer, const SDL_Rect &newClip);
	~ScopedClip();

	ScopedClip(const ScopedClip &) = delete;
	ScopedClip(ScopedClip &&) = delete;
	ScopedClip &operator=(const ScopedClip &) = delete;
	ScopedClip &operator=(ScopedClip &&) = delete;

  private:
	SDL_Renderer *rendererPtr;
	SDL_Rect oldClip;
	bool wasClipping = false;
};

template <typename Func>
void withClipRect(SDL_Renderer *renderer, const SDL_Rect &clipRect, Func function)
{
	ScopedClip guard(renderer, clipRect);
	function();
}

SDL_Rect rectFromFRect(const SDL_FRect &source);
void setColorMod(RenderItem &renderItem, SDL_Color colorMod);
void applyScalingFactor(RenderItem &renderItem, float scalingFactor);
glm::vec2 getScaledVec(glm::vec2 vect2D, float scaling);

std::optional<RenderItem> createSpriteItem(const SpriteRenderContext &ctx, const SDL_FRect &viewport,
                                           float globalScaling = 1.f);
SDL_FRect getTileDstFRect(const DstRectContext &ctx, float globalScaling = 1.f);
void sortRenderItems(std::vector<RenderItem> &renderItems);
void submitRenderItems(std::vector<RenderItem> &renderItems, const Renderer &renderer);

} // namespace GameEngine::Utils::RenderUtils