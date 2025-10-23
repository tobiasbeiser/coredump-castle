#include "Engine/GameEngine.hpp"
#include "Engine/DebugSystem.hpp"

constexpr float stepSize = 1.0f / 240.0f;


int main()
{
	const int windowWidth = 1920;
	const int windowHeight = 1080;
	GameEngine::GameEngine game("Coredump Castle", Resolution{.width=windowWidth, .height=windowHeight});
	if (!game.Init())
		return EXIT_FAILURE;

	Uint64 prevTime = SDL_GetTicks();

	float accumulator = 0.0f;
	while (game.Running()) {
		const Uint64 currentTime = SDL_GetTicks();
		const float deltaTime = static_cast<float>(currentTime - prevTime) / 1000.0f;

		//DebugSystem::Debug(std::to_string(1000 / deltaTime));

		game.HandleEvents();

		accumulator += deltaTime;
		while (accumulator >= stepSize) {
			accumulator -= stepSize;
			game.Update(stepSize);
		}
		game.Render();
		prevTime = currentTime;
	}
	game.Cleanup();
	return EXIT_SUCCESS;
}