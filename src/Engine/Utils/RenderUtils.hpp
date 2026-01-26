#pragma once
#include <SDL3/SDL_rect.h>

namespace GameEngine::Utils::RenderUtils {

SDL_Rect rectFromFrect(const SDL_FRect &source);

}