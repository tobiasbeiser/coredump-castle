#include "GameEngine.hpp"
#include "../Game/States/PlayState.hpp"
#include "GameState.hpp" 
#include "DebugSystem.hpp"

GameEngine::GameEngine(const std::string& title, int w, int h) :renderer(title, w, h), textureManager(renderer)
{
	width = w;
	height = h;
}

GameEngine::~GameEngine()
{
	Cleanup();
}


bool GameEngine::Init()
{
	// game data
	states.emplace_back(std::make_unique<PlayState>(inputManager, textureManager, renderer, physicsManager,collisionSystem));
	running = true;

	states.back()->Init();

	return true;
}


void GameEngine::Cleanup() noexcept
{
}



void GameEngine::ChangeState(std::unique_ptr<GameState> state)
{
	// cleanup the current state
	if (!states.empty()) {
		states.back()->Cleanup();
		states.pop_back();
	}

	// store and init the new state
	states.push_back(std::move(state));
	states.back()->Init();
}

void GameEngine::PushState(std::unique_ptr<GameState> state)
{
	// pause current state
	if (!states.empty()) {
		states.back()->Pause();
	}

	// store and init the new state
	states.push_back(std::move(state));
	states.back()->Init();
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
	SDL_Event event{ 0 };
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_EVENT_QUIT:
			running = false;
			break;

		case SDL_EVENT_WINDOW_RESIZED:
			renderer.Resize(event.window.data1, event.window.data2);
			break;

		case SDL_EVENT_KEY_DOWN:
			inputManager.Update(&event.key);
			break;
		case SDL_EVENT_KEY_UP:
			inputManager.Update(&event.key);
			break;
		default:
			break;
		};

	}

	if (!states.empty())
		states.back()->HandleEvents(*this);
}

void GameEngine::Update(const float& deltaTime, const float stepSize)
{
	DebugSystem::Clear();
	if (!states.empty())
		states.back()->Update(*this, deltaTime, stepSize);
}



void GameEngine::Render() {
	if (!states.empty())
		states.back()->Render(*this);
};



