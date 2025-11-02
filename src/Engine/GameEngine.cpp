#include "GameEngine.hpp"
#include "../Game/States/PlayState.hpp"
#include "DebugSystem.hpp"
#include "GameState.hpp"

namespace GameEngine {

GameEngine::GameEngine(const std::string &title, Resolution resolution)
    : renderer(title, resolution), texture_manager(renderer) {};

GameEngine::~GameEngine()
{
	Cleanup();
}

bool GameEngine::Init()
{
	// game data
	states.emplace_back(std::make_unique<PlayState>(input_manager, texture_manager, renderer));
	running = true;

	states.back()->Init(*this);

	return true;
}

void GameEngine::Cleanup() noexcept {}

void GameEngine::ChangeState(std::unique_ptr<GameState> state)
{
	// cleanup the current state
	if (!states.empty()) {
		states.back()->Cleanup();
		states.pop_back();
	}

	// store and init the new state
	states.push_back(std::move(state));
	states.back()->Init(*this);
}

void GameEngine::PushState(std::unique_ptr<GameState> state)
{
	// pause current state
	if (!states.empty()) {
		states.back()->Pause();
	}

	// store and init the new state
	states.push_back(std::move(state));
	states.back()->Init(*this);
}

void GameEngine::PopState()
{
	// cleanup the current state
	if (!states.empty()) {
		states.back()->Cleanup();
		states.pop_back();
	}

	// resume previous state
	if (!states.empty()) {
		states.back()->Resume();
	}
}

void GameEngine::HandleEvents()
{
	SDL_Event event{0};
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_EVENT_QUIT:
			running = false;
			break;

		case SDL_EVENT_WINDOW_RESIZED:
			renderer.Resize({.width = event.window.data1, .height = event.window.data2});
			break;

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			input_manager.Update(&event.key);
			break;
		default:
			break;
		};
	}

	if (!states.empty())
		states.back()->HandleEvents(*this);
}

void GameEngine::Update(const float deltaTime)
{
	DebugSystem::Clear();
	for (const auto &state : states) {
		state->Update(*this, deltaTime);
	}
}

void GameEngine::Render(const float alpha)
{
	constexpr int red = 12, green = 19, blue = 25;
	constexpr int opacity = 255;
	renderer.StartFrame(red, green, blue, opacity);
	for (const auto &state : states) {
		state->Render(*this, alpha);
	}
	renderer.EndFrame();
};
} // namespace GameEngine