#pragma once
#include "glm/vec2.hpp"

struct RenderComponent {
	int layer = 0;
	bool flipX = false;
	float scale = 1.0f;
};