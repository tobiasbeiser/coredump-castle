#pragma once
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <glm/vec2.hpp>
#include <string>
#include <vector>

struct DebugRenderOptions {
	bool showBoundingBoxes = false;
	bool showHitBoxes = true;
	bool showRayasts = true;
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

struct DebugLine {
	glm::vec2 start{};
	glm::vec2 end{};
	SDL_Color color{};
};

class DebugSystem {
  public:
	DebugRenderOptions render;
	DebugPlayerOptions player;
	std::vector<DebugRect> debugRects{};
	std::vector<DebugLine> debugLines{};
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
		instance().debugLines.clear();
		instance().debugMessages.clear();
	}
};