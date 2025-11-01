#pragma once
#include "Engine/DebugSystem.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/Renderer.hpp"

namespace GameEngine::ECS::Systems {
class RenderSystem {
  public:
	explicit RenderSystem(Renderer &renderer);
	void Update(Registry &registry, float alpha);

  private:
	struct RenderItem {
		int Layer = 0;
		SDL_Texture *texture = nullptr;
		std::optional<SDL_FRect> src;
		std::optional<SDL_FRect> dst;
		bool parallax = false;
		float scrollFactor = 0;
		bool flipX = false;
	};
	std::vector<RenderItem> renderItems;

	Renderer &renderer;

	void CenterCamera(Registry &registry) const;

};
} // namespace GameEngine::ECS::Systems