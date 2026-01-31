#pragma once
#include "Engine/Debug/DebugSystem.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/Renderer.hpp"
#include "Engine/Utils/RenderUtils.hpp"

#include <optional>

namespace GameEngine::ECS::Systems {

class RenderSystem {
	using RenderItem = Utils::RenderUtils::RenderItem;
	using BackgroundRenderContext = Utils::RenderUtils::BackgroundRenderContext;

  public:
	explicit RenderSystem(Renderer &renderer);

	RenderSystem(const RenderSystem &) = delete;
	RenderSystem &operator=(RenderSystem &&) = delete;
	RenderSystem(RenderSystem &&) = delete;
	RenderSystem &operator=(const RenderSystem &) = delete;
	~RenderSystem() = default;

	void update(Registry &registry);

  private:
	std::vector<RenderItem> renderItems{};

	Renderer &renderer;

	void centerCamera(Registry &registry) const;
	[[nodiscard]] static std::optional<RenderItem> createBackgroundItem(const BackgroundRenderContext &ctx);
};
} // namespace GameEngine::ECS::Systems