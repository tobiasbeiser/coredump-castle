#include "MainMenuState.hpp"
#include "Engine/GameEngine.hpp"
#include "Game/Factories/WorldFactory.hpp"
#include <SDL3_ttf/SDL_ttf.h>

#include "PlayState.hpp"

namespace GameEngine {

void MainMenuState::init(GameEngine &game)
{
	//Font is designed for size 20, use only multiples for crips fonts
	guiManager = std::make_unique<GuiManager>(renderer,
		fontManager.getFont("assets/fonts/upheaval/upheavtt.ttf", 20));

	titleFont = fontManager.getFont("assets/fonts/upheaval/upheavtt.ttf", 60);


	titleText = renderer.createText(titleFont, "Coredump Castle");

	Resolution res = renderer.getLogicalResolution();

	titleRect = {
		0.0f,
		floorf(res.height * 0.1f),
		static_cast<float>(res.width),
		res.height * 0.2f
	};

	// 4. Create Buttons via Manager
	// Place them below the title
	const float startY = res.height * 0.4f;
	constexpr float stepY = 50.0f; // Gap between buttons

	guiManager->createButton("Start New Game", startY, [&game, this]() {
		game.pushState(std::make_unique<PlayState>(inputManager, textureManager, renderer));
	});

	guiManager->createButton("Continue", startY + stepY, []() {
		SDL_Log("Continue");
	});

	guiManager->createButton("Options", startY + stepY * 2, []() {
		SDL_Log("Options");
	});

	guiManager->createButton("Quit", startY + stepY * 3, []
	{
		SDL_Event quit = { SDL_EVENT_QUIT };
		SDL_PushEvent(&quit);
	});
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
	guiManager->handleEvents(event);
}

void MainMenuState::update(GameEngine &game, const float deltaTime) {}

void MainMenuState::render(GameEngine &game, const float alpha)
{

	if (titleText) {
		renderer.drawTextCentered(
			titleText.get(),
			titleRect.x,
			titleRect.y,
			titleRect.w,
			titleRect.h,
			{255, 215, 0, 255} // Gold/Yellow Color
		);
	}

	if (guiManager) {
		guiManager->render();
	}



}
} // namespace GameEngine