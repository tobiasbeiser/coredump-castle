#pragma once
#include "Base/Widget.hpp"
#include "Engine/Graphics/Renderer.hpp"

#include <SDL3/SDL_events.h>
#include <glm/vec2.hpp>

namespace Ui {
class Widget;
}

namespace Ui::Utils {

inline void passMouseEvent(Widget *targetWidget, const SDL_Event &event, const Renderer &renderer)
{

	if (event.type != SDL_EVENT_MOUSE_MOTION && event.type != SDL_EVENT_MOUSE_BUTTON_DOWN
	    && event.type != SDL_EVENT_MOUSE_BUTTON_UP && event.type != SDL_EVENT_MOUSE_WHEEL) {
		return;
	}

	float mx = 0, my = 0;

	// Extract raw Window coordinates based on event type
	if (event.type == SDL_EVENT_MOUSE_MOTION) {
		mx = event.motion.x;
		my = event.motion.y;
	} else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
		mx = event.wheel.mouse_x;
		my = event.wheel.mouse_y;
	} else {
		mx = event.button.x;
		my = event.button.y;
	}

	// CONVERT to Logical Coordinates
	const glm::vec2 mouseLogicalPos = renderer.windowToLogical(mx, my);
	targetWidget->handleEvent(mouseLogicalPos, event);
}
} // namespace Ui::Utils