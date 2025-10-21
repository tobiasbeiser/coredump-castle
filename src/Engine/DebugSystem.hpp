#pragma once
#include <string>
#include <vector>

struct DebugRenderOptions {
	bool showBoundingBoxes = true;
};

struct DebugPlayerOptions {
	bool showState = true;

	bool godMode = false;
	bool noClip = false;
};

struct DebugRect {
	SDL_FRect rect;
	SDL_Color color;
	
};

class DebugSystem {
  public:
	DebugRenderOptions render;
	DebugPlayerOptions player;
	std::vector<DebugRect> debugRects;
	std::vector<std::string> debugMessages;

	static DebugSystem &Instance()
	{
		static DebugSystem instance;
		return instance;
	}

	static void Debug(const std::string &message) { Instance().debugMessages.push_back(message); }

	static void Clear()
	{
		Instance().debugRects.clear();
		Instance().debugMessages.clear();
	}
};