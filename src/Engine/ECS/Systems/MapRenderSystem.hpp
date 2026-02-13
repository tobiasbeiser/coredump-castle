#pragma once
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/Gui/LayoutEngine.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Utils/RenderUtils.hpp"

#include <vector>

namespace GameEngine {
class GameEngine;
}

namespace GameEngine::ECS::Systems {

class MapRenderSystem {
	using RenderItem = Utils::RenderUtils::RenderItem;

  public:
	explicit MapRenderSystem(Renderer &renderer, const float scalingFactor)
	    : renderer(renderer), scalingFactor(scalingFactor)
	{
		const auto [width, height] = renderer.getLogicalResolution();
		const glm::vec2 resVector{width, height};
		const SDL_FRect targetRect = Ui::LayoutEngine::positionCentered(resVector, resVector * mapSizeFactor);
		// SDL3 only allows clipping with integer rects https://github.com/libsdl-org/sdl/issues/14654
		clipRect = Utils::RenderUtils::rectFromFRect(targetRect);
	}

	void update(Registry &registry, GameEngine &game);
	void initFocusPoint(Registry &registry);
	void setMapClipRect(SDL_FRect newClipRect);
	static float getMapSizeFactor();
	void handleZoom(float wheelAmount);

  private:
	static constexpr float mapSizeFactor = 0.80f;
	std::vector<Utils::RenderUtils::RenderItem> renderItems{};
	Renderer &renderer;
	float scalingFactor;
	glm::vec2 focusPoint{0.f, 0.f};
	static constexpr SDL_Color invalidColorMod{100, 100, 100, 200};
	SDL_Rect clipRect{};
};

} // namespace GameEngine::ECS::Systems