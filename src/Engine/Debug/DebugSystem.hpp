#pragma once
#include "Engine/ECS/Registry.hpp"
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <string>
#include <vector>
#include <array>

struct DebugRenderOptions {
	bool showBoundingBoxes = false;
	bool showHitBoxes = false;
	bool showTooltipBoxes = false;
};

struct DebugSkipMenuOptions {
	bool skipMainMenu = false;
};

struct DebugPlayerOptions {
	bool showState = false;
	bool showPosition = true;
	bool godMode = false;
	bool noClip = false;
};

struct DebugRect {
	SDL_FRect rect{};
	SDL_Color color{};
};

class DebugSystem {
  public:
	static DebugSystem &instance()
	{
		static DebugSystem instance;
		return instance;
	}

	static void debug(const std::string &message);
	static void addRect(const SDL_FRect &rect, const SDL_Color &color);
	static void clear();

	static void renderGameOverlay(SDL_Renderer *renderer, const SDL_FRect &viewport);
	static void drawUI();

	void toggle() { enabled = !enabled; }
	void setRegistry(GameEngine::ECS::Registry &registryPtr) { registry = &registryPtr; }

	const DebugRenderOptions &getRenderOptions() const { return render; }
	const DebugPlayerOptions &getPlayerOptions() const { return player; }
	const DebugSkipMenuOptions &getSkipMenuOptions() const { return skipMenu; }

  private:
	bool enabled = false;
	std::vector<DebugRect> debugRects{};
	std::vector<std::string> debugMessages{};
	DebugRenderOptions render;
	DebugPlayerOptions player;
	DebugSkipMenuOptions skipMenu;
	GameEngine::ECS::Registry *registry = nullptr;
	void drawCollisionBoxes() const;
	void drawHitBoxes() const;
	void drawTooltopBoxes() const;
	void updateFPS();

	static constexpr int fpsHistorySize = 400;
	std::array<float, fpsHistorySize> fpsHistory{};
	int fpsOffset = 0;
};