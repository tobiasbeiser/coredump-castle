#include "MainMenuState.hpp"
#include "Engine/GameEngine.hpp"

#include "PlayState.hpp"
#include "Engine/Graphics/Gui/CoredumpContext.hpp"
#include "Engine/Graphics/Gui/UiViewFactory.hpp"

namespace GameEngine {

void MainMenuState::init(GameEngine &game)
{
	// TODO: add debug skip to init game
	if (DebugSystem::instance().getSkipMenuOptions().skipMainMenu) {
		game.pushState(std::make_unique<PlayState>(game.getInputManager(), game.getTextureManager(), game.getRenderer()));
	}
	context = CoredumpContext::getContext(game);

	guiManager = std::make_unique<Ui::GuiManager>();
	setupUI(game);
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

void MainMenuState::handleEvents(GameEngine &game, const SDL_Event& event)
{
	guiManager->handleEvent(*context, event);
}

void MainMenuState::update(GameEngine &game, const float deltaTime) {}

void MainMenuState::render(GameEngine &game, const float alpha)
{

	if (guiManager) {
		guiManager->render(*context);
	}

}

void MainMenuState::setupUI(GameEngine& game)
{

	MainMenuActions actions;

	actions.onStartNewGame = [&game]() {
		game.pushState(std::make_unique<PlayState>(game.getInputManager(), game.getTextureManager(), game.getRenderer()));
	};

	 if (!game.getSaveSystem().getSaves().empty())
	 {
		 actions.onShowContinue = [this, &game]() {
		 	ContinueMenuActions contActions;

		 	contActions.onContinue = [&game](const std::string_view saveName) {
		 		game.getSaveSystem().setCurrentSaveName(saveName);
		 		game.pushState(std::make_unique<PlayState>(game.getInputManager(), game.getTextureManager(), game.getRenderer()));
		 	};

		 	contActions.onBack = [this, &game]() { setupUI(game); }; // Recursive call to rebuild MainMenu

		 	const auto gameSaves = game.getSaveSystem().getSaves();
		 	auto continueView = UiViewFactory::createContinueMenu(*context, contActions, gameSaves);
		 	guiManager->setRoot(std::move(continueView));

		 	const auto [width, height] = renderer.getLogicalResolution();
		 	guiManager->updateLayout(*context, width , height);
		 };
	 }


	// switch the UI, but remain in MainMenuState
	actions.onShowOptions = [this, &game]() {
		OptionsMenuActions optActions;
		optActions.onBack = [this, &game]() { setupUI(game); }; // Recursive call to rebuild MainMenu

		auto optionsView = UiViewFactory::createOptionsMenu(*context, optActions);
		guiManager->setRoot(std::move(optionsView));
		const auto [width, height] = renderer.getLogicalResolution();
		guiManager->updateLayout(*context, width , height);
	};

	actions.onQuit = []() {
		SDL_Event quit = { SDL_EVENT_QUIT };
		SDL_PushEvent(&quit);
	};

	auto mainView = UiViewFactory::createMainMenu(*context, actions);

	this->guiManager->setRoot(std::move(mainView));

	Resolution resolution = renderer.getLogicalResolution();
	guiManager->updateLayout(*context, resolution.width , resolution.height);
}
} // namespace GameEngine