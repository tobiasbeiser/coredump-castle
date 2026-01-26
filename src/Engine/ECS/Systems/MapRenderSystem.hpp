#pragma once
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/Gui/LayoutEngine.hpp"
#include "Engine/Graphics/Gui/Layouts/AnchorLayout.hpp"
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
		clipRect = Utils::RenderUtils::rectFromFRect(targetRect);
	}

	void update(Registry &registry, GameEngine &game);
	void centerCamera(Registry &registry) const;
	void setMapClipRect(SDL_FRect newClipRect);
	static float getMapSizeFactor();
	void handleZoom(float wheelAmount);

  private:
	static constexpr float mapSizeFactor = 0.80f;
	std::vector<Utils::RenderUtils::RenderItem> renderItems{};
	Renderer &renderer;
	float scalingFactor;
	static constexpr SDL_Color invalidColorMod{100, 100, 100, 200};
	SDL_Rect clipRect{};
};

} // namespace GameEngine::ECS::Systems