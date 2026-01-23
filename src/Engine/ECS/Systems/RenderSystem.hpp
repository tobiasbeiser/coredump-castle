#pragma once
#include "../../Debug/DebugSystem.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/Renderer.hpp"

namespace GameEngine::ECS::Systems {
class RenderSystem {
  public:
	explicit RenderSystem(Renderer &renderer);

	RenderSystem(const RenderSystem &) = delete;
	RenderSystem &operator=(RenderSystem &&) = delete;
	RenderSystem(RenderSystem &&) = delete;
	RenderSystem &operator=(const RenderSystem &) = delete;
	~RenderSystem() = default;

	void update(Registry &registry, float alpha);

  private:
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
	std::vector<RenderItem> renderItems{};

	Renderer &renderer;

	void centerCamera(Registry &registry) const;
};
} // namespace GameEngine::ECS::Systems