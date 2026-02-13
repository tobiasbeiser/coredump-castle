#pragma once
#include "Engine/ECS/Components/Gameplay/TooltipComponent.hpp"
#include "Engine/ECS/Components/Render/TextDisplayComponent.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Graphics/Renderer.hpp"

namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems {
class UiRenderSystem {
  public:
	explicit UiRenderSystem(Renderer &renderer);
	void update(Registry &registry, const GameEngine &game) const;
	void initTextDisplayComp(const std::string &text, TextDisplayComponent &textDisplay, const GameEngine &game) const;

  private:
	Renderer &renderer;
	glm::vec2 resolutionSize;
};

} // namespace GameEngine::ECS::Systems
