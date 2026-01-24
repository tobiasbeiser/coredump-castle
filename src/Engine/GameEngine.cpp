#include "GameEngine.hpp"
#include "../Game/States/PlayState.hpp"
#include "Debug/DebugSystem.hpp"
#include "FileIO/FileSaveStorage.hpp"
#include "Game/States/MainMenuState.hpp"
#include "GameState.hpp"

#include <imgui_impl_sdl3.h>

namespace GameEngine {

GameEngine::GameEngine(const std::string &title, const Resolution resolution)
    : renderer(title, resolution), textureManager(renderer), fontManager(renderer),
      saveSystem(std::make_unique<ECS::FileSaveStorage>())
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
	states.emplace_back(std::make_unique<MainMenuState>(inputManager, textureManager, fontManager, renderer));
	running = true;

	states.back()->init(*this);

	return true;
}

void GameEngine::cleanup() noexcept {}

void GameEngine::changeState(std::unique_ptr<GameState> state)
{
	actionQueue.push_back({PendingAction::Type::Change, std::move(state)});
}

void GameEngine::pushState(std::unique_ptr<GameState> state)
{
	actionQueue.push_back({PendingAction::Type::Push, std::move(state)});
}

void GameEngine::popState()
{
	actionQueue.push_back({PendingAction::Type::Pop});
}

void GameEngine::handleEvents()
{
	PROFILE_SCOPE("GameEngine::handleEvents");
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
			if (!states.empty())
				states.back()->handleEvents(*this, event);
			break;
		};
	}
}

void GameEngine::update(const float deltaTime)
{
	PROFILE_SCOPE("GameEngine::Update");
	DebugSystem::clear();
	for (const auto &state : states) {
		state->update(*this, deltaTime);
	}

	for (auto &action : actionQueue) {
		switch (action.type) {
		case PendingAction::Type::Change:
			if (!states.empty()) {
				states.back()->cleanup();
				states.pop_back();
			}
			if (action.state) {
				states.push_back(std::move(action.state).value());
				states.back()->init(*this);
			}
			break;

		case PendingAction::Type::Push:
			if (!states.empty()) {
				states.back()->pause();
			}
			if (action.state) {
				states.push_back(std::move(action.state).value());
				states.back()->init(*this);
			}
			break;

		case PendingAction::Type::Pop:
			if (!states.empty()) {
				states.back()->cleanup();
				states.pop_back();
			}
			if (!states.empty()) {
				states.back()->resume();
			}
			break;
		}
	}
	actionQueue.clear();
}

void GameEngine::render(const float alpha)
{
	PROFILE_SCOPE("GameEngine::Render");
	constexpr int red = 12, green = 19, blue = 25;
	constexpr int opacity = 255;
	renderer.startFrame(red, green, blue, opacity);
	for (const auto &state : states) {
		state->render(*this, alpha);
	}
	renderer.endFrame();
};
} // namespace GameEngine