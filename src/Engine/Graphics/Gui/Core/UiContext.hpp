#pragma once
#include "Engine/Graphics/Renderer.hpp"

namespace Ui {
struct Paint {
	SDL_Color fillCol{};
	SDL_Color outlineCol{};
	SDL_Color textCol{};
};

struct ButtonStyle {
	float widthPadding = 5.0f;
	float heightPadding = 5.0f;
	Paint normal;
	Paint hovered;
	Paint pressed;
	Paint disabled;
};

struct FrameStyle {
	SDL_Color fill{};
	SDL_Color outline{};
};

struct UiTheme {
	SDL_Color titleTextColor{};
	ButtonStyle button{};
	FrameStyle frame{};
};

struct UIContext {
	Renderer *renderer = nullptr;
	TTF_Font *titleFont = nullptr;
	TTF_Font *bigFont = nullptr;
	TTF_Font *textFont = nullptr;
	UiTheme theme;
	Resolution resolution{};
};
} // namespace Ui
