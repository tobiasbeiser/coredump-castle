#include "PauseMenuState.hpp"

#include "Engine/GameEngine.hpp"
#include "Engine/Graphics/Gui/UiViewFactory.hpp"
#include "Engine/Graphics/Gui/Utils.hpp"
#include "MainMenuState.hpp"
#include "PlayState.hpp"
#include "Engine/ECS/Systems/MenuStateToggleSystem.hpp"

namespace GameEngine {

void PauseMenuState::init(GameEngine &game)
{
	setupUI(game);
}

void PauseMenuState::cleanup() {}

void PauseMenuState::pause()
{
	paused = true;
}

void PauseMenuState::resume()
{
	paused = false;
}

void PauseMenuState::handleEvents(GameEngine &game, const SDL_Event &event)
{
	PROFILE_SCOPE("PauseMenuState::handleEvents");
	Ui::Utils::passMouseEvent(rootWidget.get(), event, renderer);
}

void PauseMenuState::update(GameEngine &game, const float deltaTime)
{
	if (paused) {
		return;
	}
	inputSystem.update(game.getRegistry());
	ECS::Systems::MenuStateToggleSystem::update(game.getRegistry(), game);
}

void PauseMenuState::render(GameEngine &game, const float alpha)
{
	PROFILE_SCOPE("PauseMenuState::render");
	rootWidget->draw(guiManager.getContext(), {0, 0});
}

void PauseMenuState::setupUI(GameEngine &game)
{
	PauseMenuActions actions;

	actions.onShowSettings = [this, &game]() { showSettingsMenu(game); };
	actions.onShowInventory = [this, &game]() { showInventory(game); };

	actions.onBackToMainMenu = [&game]() {
		game.getRegistry().clearAll();
		game.getSaveSystem().clearCurrentSaveName();
		game.resetToState(std::make_unique<MainMenuState>(game.getInputManager(), game.getTextureManager(), game.getGuiManager(), game.getRenderer()));
	};

	rootWidget = UiViewFactory::createPauseMenu(guiManager.getContext(), actions);

	guiManager.updateLayout(rootWidget.get());
}

void PauseMenuState::showSettingsMenu(GameEngine &game)
{
	SettingsMenuActions optActions;
	optActions.onBack = [this, &game]() { setupUI(game); };

	rootWidget = UiViewFactory::createSettingsMenu(guiManager.getContext(), optActions, true);
	guiManager.updateLayout(rootWidget.get());
}

void PauseMenuState::showInventory(GameEngine &game)
{
	InventoryActions invActions;

	invActions.onBack = [this, &game]() { setupUI(game); };

	auto playerView = game.getRegistry().view<PlayerTag, InventoryComponent, ProgressComponent>();
	if (playerView.size() != 1) {
		return;
	}
	const auto playerEntity = playerView.at(0U);

	const auto &inventory = playerView.get<InventoryComponent>(playerEntity);
	const auto &progress = playerView.get<ProgressComponent>(playerEntity);

	rootWidget = UiViewFactory::createInventoryMenu(guiManager.getContext(), invActions, inventory.keys.size(), progress.unlockedAbilities);
	guiManager.updateLayout(rootWidget.get());
}

} // namespace GameEngine
