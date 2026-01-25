#include "MainMenuState.hpp"

#include "Engine/GameEngine.hpp"
#include "Engine/Graphics/Gui/CoredumpContext.hpp"
#include "Engine/Graphics/Gui/UiViewFactory.hpp"
#include "GameLoadingState.hpp"
#include "PlayState.hpp"

namespace GameEngine {

void MainMenuState::init(GameEngine &game)
{
	if (DebugSystem::instance().getSkipMenuOptions().skipMainMenu) {
		game.pushState(std::make_unique<PlayState>(game.getInputManager(), game.getTextureManager(),
		                                           game.getGuiManager(), game.getRenderer(), game.getEffectFactory()));
	}

	setupUI(game);
	game.getAudioManager().playMusic("assets/audio/music/main_menu_theme.ogg", 2500);
}

void MainMenuState::cleanup() {}

void MainMenuState::pause()
{
	paused = true;
}

void MainMenuState::resume()
{
	paused = false;
}

void MainMenuState::handleEvents(GameEngine &game, const SDL_Event &event)
{
	PROFILE_SCOPE("MainMenuState::handleEvents");
	guiManager.handleEvent(event);
}

void MainMenuState::update(GameEngine &game, const float deltaTime) {}

void MainMenuState::render(GameEngine &game, const float alpha)
{
	PROFILE_SCOPE("MainMenuState::render");
	if (SDL_Texture *backgroundTexture = &game.getTextureManager().getTexture("assets/ui/main_menu_background.png")) {
		SDL_SetTextureColorMod(backgroundTexture, 170, 170, 170);
		SDL_SetTextureAlphaMod(backgroundTexture, 255);
		renderer.renderTexture(*backgroundTexture, std::nullopt, std::nullopt, false);
	}
	guiManager.render();
}

void MainMenuState::setupUI(GameEngine &game)
{

	MainMenuActions actions;

	actions.onStartNewGame = [&game]() { transitionToPlayState(game); };

	if (!game.getSaveSystem().getSaves().empty()) {
		actions.onShowContinue = [this, &game]() { showContinueMenu(game); };
	}

	// switch the UI, but remain in MainMenuState
	actions.onShowSettings = [this, &game]() { showSettingsMenu(game); };

	actions.onQuit = []() {
		SDL_Event quit = {SDL_EVENT_QUIT};
		SDL_PushEvent(&quit);
	};

	auto mainView = UiViewFactory::createMainMenu(guiManager.getContext(), actions);

	guiManager.setRoot(std::move(mainView));

	guiManager.updateLayout();
}

void MainMenuState::showContinueMenu(GameEngine &game)
{
	ContinueMenuActions contActions;

	contActions.onContinue = [&game](const std::string_view saveName) {
		game.getSaveSystem().setCurrentSaveName(saveName);
		transitionToPlayState(game);
	};

	contActions.onBack = [this, &game]() { setupUI(game); }; // Recursive call to rebuild MainMenu

	const auto gameSaves = game.getSaveSystem().getSaves();
	auto continueView = UiViewFactory::createContinueMenu(guiManager.getContext(), contActions, gameSaves);
	guiManager.setRoot(std::move(continueView));

	guiManager.updateLayout();
}

void MainMenuState::showSettingsMenu(GameEngine &game)
{
	SettingsMenuActions optActions;
	optActions.onBack = [this, &game]() { setupUI(game); }; // Recursive call to rebuild MainMenu
	if (!game.getSaveSystem().getSaves().empty()) {
		optActions.onDelete = [this, &game] {
			game.getSaveSystem().deleteAllSaves();
			showSettingsMenu(game); // reload to disable delete button
		};
	}

	auto optionsView = UiViewFactory::createSettingsMenu(guiManager.getContext(), optActions);
	guiManager.setRoot(std::move(optionsView));
	guiManager.updateLayout();
}

void MainMenuState::transitionToPlayState(GameEngine &game)
{
	auto playStateFactory = [&game]() {
		return std::make_unique<PlayState>(game.getInputManager(), game.getTextureManager(), game.getGuiManager(),
		                                   game.getRenderer(), game.getEffectFactory());
	};
	game.changeState(std::make_unique<GameLoadingState>(std::move(playStateFactory), game.getGuiManager()));
}

} // namespace GameEngine