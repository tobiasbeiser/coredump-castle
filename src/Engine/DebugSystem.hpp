#pragma once
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <string>
#include <vector>

struct DebugRenderOptions {
	bool showBoundingBoxes = false;
	bool showHitBoxes = false;
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

	static void debug(const std::string &message);
	static void clear();

	static void renderGameOverlay(SDL_Renderer *renderer, const SDL_FRect &viewport);
	static void drawUI();
};