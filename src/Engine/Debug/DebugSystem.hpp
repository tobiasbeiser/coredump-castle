#pragma once
#include "Engine/ECS/Registry.hpp"
#include "Engine/Graphics/Renderer.hpp"

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <array>
#include <glm/vec2.hpp>
#include <string>
#include <vector>

struct DebugRenderOptions {
	bool showBoundingBoxes = false;
	bool showHitBoxes = false;
	bool showRaycasts = false;
	bool showTooltipBoxes = false;
	bool vSync = true;
};

struct DebugSkipMenuOptions {
	bool skipMainMenu = false;
};

struct DebugPlayerOptions {
	bool showState = false;
	bool showPosition = true;
	bool showHP = true;
};

struct Cheats {
	bool godMode = false;
	bool noClip = false;
	bool unlimitedJumps = false;
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
	static DebugSystem &instance()
	{
		static DebugSystem instance;
		return instance;
	}

	static void debug(const std::string &message);
	static void addRect(const SDL_FRect &rect, const SDL_Color &color);
	static void addLine(const glm::vec2 &start, const glm::vec2 &end, const SDL_Color &color);
	static void addRaycast(const glm::vec2 &start, const glm::vec2 &end, bool obstructed);
	static void clear();

	static void renderGameOverlay(SDL_Renderer *renderer, const SDL_FRect &viewport);
	static void updateSettings(SDL_Renderer *renderer);
	static void drawUI();

	void toggle() { enabled = !enabled; }
	void setRegistry(GameEngine::ECS::Registry &registryPtr) { registry = &registryPtr; }

	const DebugRenderOptions &getRenderOptions() const { return render; }
	const DebugPlayerOptions &getPlayerOptions() const { return player; }
	const DebugSkipMenuOptions &getSkipMenuOptions() const { return skipMenu; }
	const Cheats &getCheats() const { return cheats; }

  private:
	bool enabled = false;
	std::vector<DebugRect> debugRects{};
	std::vector<DebugLine> debugLines{};
	std::vector<std::string> debugMessages{};
	DebugRenderOptions render;
	DebugPlayerOptions player;
	Cheats cheats;
	DebugSkipMenuOptions skipMenu;
	GameEngine::ECS::Registry *registry = nullptr;
	void drawCollisionBoxes() const;
	void drawProjectileHitBox() const;
	void drawPatrolBoxes(std::vector<unsigned>::value_type entity, SDL_FRect hurtBoxRect) const;
	void drawHitBoxes() const;
	void drawTooltipBoxes() const;
	void updateFPS();

	static constexpr int fpsHistorySize = 400;
	std::array<float, fpsHistorySize> fpsHistory{};
	int fpsOffset = 0;
};