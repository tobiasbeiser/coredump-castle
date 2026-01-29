
#include "UiRenderSystem.hpp"

#include "Engine/GameEngine.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/ECS/Components/Gameplay/TooltipComponent.hpp"
#include "Engine/ECS/Components/Render/TextDisplayComponent.hpp"
#include "Engine/Graphics/Gui/LayoutEngine.hpp"
#include "Engine/Graphics/Gui/Layouts/AnchorLayout.hpp"

GameEngine::ECS::Systems::UiRenderSystem::UiRenderSystem(Renderer &renderer): renderer(renderer)
{
	const auto [width, height] = renderer.getLogicalResolution();
	resolutionSize = {width, height};
}

void GameEngine::ECS::Systems::UiRenderSystem::update(Registry &registry, const GameEngine &game) const
{
	for (auto textView = registry.view<TooltipComponent, TextDisplayComponent>(); const auto entity : textView) {
		auto &textDisplay = textView.get<TextDisplayComponent>(entity);
		auto &tooltip = textView.get<TooltipComponent>(entity);

		if (!textDisplay.textObject) {
			initTextDisplayComp(tooltip, textDisplay, game);
		}

		renderer.drawText(textDisplay.textObject.get(), textDisplay.position.x, textDisplay.position.y);
	}
}

void GameEngine::ECS::Systems::UiRenderSystem::initTextDisplayComp(const TooltipComponent &tooltip, TextDisplayComponent &textDisplay, const GameEngine &game) const
{
	textDisplay.textObject = renderer.createText(game.getGuiManager().getContext().textFont, tooltip.text);
	const auto textSize = renderer.measureText(textDisplay.textObject.get());
	const SDL_FRect anchoredPos = Ui::LayoutEngine::positionAnchored(resolutionSize, textSize, Ui::AnchorParams{{0.5f, 1.f}, {0.5f, 1.f}, {0.f, -10.f}});
	textDisplay.position = {anchoredPos.x, anchoredPos.y};
}