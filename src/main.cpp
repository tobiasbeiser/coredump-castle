#include "Engine/DebugSystem.hpp"
#include "Engine/GameEngine.hpp"
#include "math.h"

int main()
{
	constexpr int windowWidth = 1920;
	constexpr int windowHeight = 1080;
	GameEngine::GameEngine game("Coredump Castle", Resolution{.width = windowWidth, .height = windowHeight});
	if (!game.init())
		return EXIT_FAILURE;

	Uint64 prevTime = SDL_GetTicks();

	while (game.isRunning()) {
		const Uint64 currentTime = SDL_GetTicks();
		const float deltaTime = static_cast<float>(currentTime - prevTime) / 1000.0f;
		prevTime = currentTime;

		game.handleEvents();
		game.update(deltaTime);
		game.render(deltaTime);
	}
	game.cleanup();
	return EXIT_SUCCESS;
}