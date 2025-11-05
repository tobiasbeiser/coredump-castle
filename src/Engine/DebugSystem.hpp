#pragma once
#include <string>
#include <vector>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_pixels.h>

struct DebugRenderOptions {
	bool showBoundingBoxes = false;
};

struct DebugPlayerOptions {
	bool showState = false;

	bool godMode = false;
	bool noClip = false;
};

struct DebugRect {
	SDL_FRect rect{};
	SDL_Color color{};
	
};

class DebugSystem {
  public:
	DebugRenderOptions render;
	DebugPlayerOptions player;
	std::vector<DebugRect> debugRects{};
	std::vector<std::string> debugMessages{};

	static DebugSystem &instance()
	{
		static DebugSystem instance;
		return instance;
	}

	static void debug(const std::string &message) { instance().debugMessages.push_back(message); }

	static void clear()
	{
		instance().debugRects.clear();
		instance().debugMessages.clear();
	}
};