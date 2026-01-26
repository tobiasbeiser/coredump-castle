
#include "RenderUtils.hpp"

SDL_Rect GameEngine::Utils::RenderUtils::rectFromFrect(const SDL_FRect &source)
{
	return SDL_Rect{
	    static_cast<int>(source.x),
	    static_cast<int>(source.y),
	    static_cast<int>(source.w),
	    static_cast<int>(source.h),
	};
}
