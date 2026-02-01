#pragma once
#include <SDL3_ttf/SDL_ttf.h>
#include <glm/vec2.hpp>
#include <memory>
#include "Engine/Utils/Timer.hpp"

using namespace  std::chrono_literals;

struct TextDisplayComponent {
	TextDisplayComponent() = default;
	~TextDisplayComponent() = default;
	TextDisplayComponent(TextDisplayComponent&& other) noexcept = default;
	TextDisplayComponent& operator=(TextDisplayComponent&& other) noexcept = default;
	TextDisplayComponent(const TextDisplayComponent&) = delete;
	TextDisplayComponent& operator=(const TextDisplayComponent&) = delete;
	std::unique_ptr<TTF_Text, TTFTextDestroyer> textObject;
	glm::vec2 position{};
	SDL_Color textColor{255, 255, 255, 255};
	Timer typewriterTimer{std::chrono::duration<float>(2ms), false};
	int counter{1};
};