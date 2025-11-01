#pragma once
#include <SDL3/SDL_render.h>
struct BackgroundComponent {
	SDL_Texture *texture = nullptr;
	float scrollingSpeed = 0.0f;
	float scrollPos = 0.0f;
	float yOffset = 0.0f;
	bool repeatX = false;
	int levelId = 0;
};
