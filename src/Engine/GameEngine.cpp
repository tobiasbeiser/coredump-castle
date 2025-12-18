#include "GameEngine.hpp"
#include "../Game/States/PlayState.hpp"
#include "DebugSystem.hpp"
#include "FileIO/FileSaveStorage.hpp"
#include "Game/States/MainMenuState.hpp"
#include "GameState.hpp"

namespace GameEngine {

GameEngine::GameEngine(const std::string &title, const Resolution resolution)
    : renderer(title, resolution), textureManager(renderer), fontManager(renderer),
      saveSystem(std::make_unique<ECS::FileSaveStorage>()){};

GameEngine::~GameEngine()
{
	cleanup();
}

bool GameEngine::init()
{
	// game data
	states.emplace_back(std::make_unique<MainMenuState>(inputManager, textureManager, fontManager, renderer));
	running = true;

	states.back()->init(*this);

	return true;
}

void GameEngine::cleanup() noexcept {}

void GameEngine::changeState(std::unique_ptr<GameState> state)
{
	// cleanup the current state
	if (!states.empty()) {
		states.back()->cleanup();
		states.pop_back();
	}

	// store and init the new state
	states.push_back(std::move(state));
	states.back()->init(*this);
}

void GameEngine::pushState(std::unique_ptr<GameState> state)
{
	// pause current state
	if (!states.empty()) {
		states.back()->pause();
	}

	// store and init the new state
	states.push_back(std::move(state));
	states.back()->init(*this);
}

void GameEngine::popState()
{
	// cleanup the current state
	if (!states.empty()) {
		states.back()->cleanup();
		states.pop_back();
	}

	// resume previous state
	if (!states.empty()) {
		states.back()->resume();
	}
}

void GameEngine::handleEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT)
		{
			running = false;
			return;
		}
		if (event.type == SDL_EVENT_WINDOW_RESIZED)
		{
			renderer.resize({.width = event.window.data1, .height = event.window.data2});
			continue;
		}

		if (!states.empty())
		{
			states.back()->handleEvents(*this, event);
		}
	}
}

void GameEngine::update(const float deltaTime)
{
	DebugSystem::clear();
	for (const auto &state : states) {
		state->update(*this, deltaTime);
	}
}

void GameEngine::render(const float alpha)
{
	constexpr int red = 12, green = 19, blue = 25;
	constexpr int opacity = 255;
	renderer.startFrame(red, green, blue, opacity);
	for (const auto &state : states) {
		state->render(*this, alpha);
	}
	renderer.endFrame();
};
} // namespace GameEngine