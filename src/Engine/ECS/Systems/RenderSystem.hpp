#pragma once
#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/ECS/Components/Core/PositionComponent.hpp"
#include "Engine/ECS/Components/Core/TransformComponent.hpp"
#include "Engine/ECS/Components/Render/BackgroundComponent.hpp"
#include "Engine/ECS/Components/Render/RenderComponent.hpp"
#include "Engine/ECS/Components/Render/SpriteComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/Renderer.hpp"

#include <optional>

namespace GameEngine::ECS::Systems {

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

class RenderSystem {
  public:
	explicit RenderSystem(Renderer &renderer);

	RenderSystem(const RenderSystem &) = delete;
	RenderSystem &operator=(RenderSystem &&) = delete;
	RenderSystem(RenderSystem &&) = delete;
	RenderSystem &operator=(const RenderSystem &) = delete;
	~RenderSystem() = default;

	void update(Registry &registry);
	void updateIngameMap(Registry &registry);

  private:
	std::vector<RenderItem> renderItems{};

	Renderer &renderer;
	void centerCamera(Registry &registry) const;
	void sortRenderItems();
	void submitRenderItems();

	[[nodiscard]] static std::optional<RenderItem> createBackgroundItem(const BackgroundRenderContext &ctx);

	[[nodiscard]] static std::optional<RenderItem> createSpriteItem(const SpriteRenderContext &ctx,
	                                                                const SDL_FRect &viewport);
};
} // namespace GameEngine::ECS::Systems