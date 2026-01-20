#include "GameEngine.hpp"
#include "../Game/States/PlayState.hpp"
#include "DebugSystem.hpp"
#include "FileIO/FileSaveStorage.hpp"
#include "GameState.hpp"

#include <imgui_impl_sdl3.h>

namespace GameEngine {

GameEngine::GameEngine(const std::string &title, const Resolution resolution)
    : renderer(title, resolution), textureManager(renderer), saveSystem(std::make_unique<ECS::FileSaveStorage>())
{
	DebugSystem::instance().setRegistry(registry);
};

GameEngine::~GameEngine()
{
	cleanup();
}

bool GameEngine::init()
{
	// game data
	states.emplace_back(std::make_unique<PlayState>(inputManager, textureManager, renderer));
	running = true;

	states.back()->init(*this);

	return true;
}

void GameEngine::cleanup() noexcept {}

void GameEngine::changeState(std::unique_ptr<GameState> state)
{
	pendingOperations.emplace_back([this, s = std::move(state)]() mutable {
		if (!states.empty()) {
			states.back()->cleanup();
			states.pop_back();
		}
		states.push_back(std::move(s));
		states.back()->init(*this);
	});
}

void GameEngine::pushState(std::unique_ptr<GameState> state)
{
	pendingOperations.emplace_back([this, s = std::move(state)]() mutable {
		if (!states.empty()) {
			states.back()->pause();
		}
		states.push_back(std::move(s));
		states.back()->init(*this);
	});
}

void GameEngine::popState()
{
	pendingOperations.emplace_back([this]() {
		if (!states.empty()) {
			states.back()->cleanup();
			states.pop_back();
		}
		if (!states.empty()) {
			states.back()->resume();
		}
	});
}

void GameEngine::handleEvents()
{
	SDL_Event event{0};
	while (SDL_PollEvent(&event)) {
		ImGui_ImplSDL3_ProcessEvent(&event);

		switch (event.type) {
		case SDL_EVENT_QUIT:
			running = false;
			break;

		case SDL_EVENT_WINDOW_RESIZED:
			renderer.resize({.width = event.window.data1, .height = event.window.data2});
			break;

		case SDL_EVENT_KEY_DOWN:
			if (event.key.key == SDLK_F3) {
				DebugSystem::instance().toggle();
			}
		case SDL_EVENT_KEY_UP:
			inputManager.processEvent(&event.key);
			break;
		default:
			break;
		};
	}

	if (!states.empty())
		states.back()->handleEvents(*this);
}

void GameEngine::update(const float deltaTime)
{
	DebugSystem::clear();
	for (const auto &state : states) {
		state->update(*this, deltaTime);
	}

	for (auto &operation : pendingOperations) {
		operation();
	}
	pendingOperations.clear();
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