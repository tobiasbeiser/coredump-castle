#include "Engine/DebugSystem.hpp"
#include "Engine/GameEngine.hpp"


int main()
{
	const int windowWidth = 1920;
	const int windowHeight = 1080;
	GameEngine::GameEngine game("Coredump Castle", Resolution{.width = windowWidth, .height = windowHeight});
	if (!game.Init())
		return EXIT_FAILURE;

	Uint64 prevTime = SDL_GetTicks();

	while (game.Running()) {
		const Uint64 currentTime = SDL_GetTicks();
		const float deltaTime = static_cast<float>(currentTime - prevTime) / 1000.0f;
		prevTime = currentTime;

		game.HandleEvents();
		game.Update(deltaTime);
		game.Render(deltaTime);
	}
	game.Cleanup();
	return EXIT_SUCCESS;
}