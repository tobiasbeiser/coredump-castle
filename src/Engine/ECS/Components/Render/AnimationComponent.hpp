#pragma once
#include "Engine/ECS/Components/Core/StateComponent.hpp"
#include "Engine/Graphics/Animation.hpp"
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <glm/vec2.hpp>
#include <unordered_map>
#include <vector>

struct AnimationFrame {
	SDL_Texture *texture{};
	glm::ivec2 tileSize{};
	glm::ivec2 tileMapPosition{};
};

struct AnimationComponent {
	std::unordered_map<int, Animation> animations{};
	std::unordered_map<int, std::vector<AnimationFrame>> frames{};

	int currentAnimation = 0;
	int previousAnimation = 0;

	void addAnimation(const State &state, const std::vector<AnimationFrame> &animationFrames, float duration)
	{
		int stateIndex = 0;
		stateIndex = static_cast<int>(state);
		animations[stateIndex] = Animation(static_cast<int>(animationFrames.size()), duration);
		frames[stateIndex] = animationFrames;
	}

	void addAnimationFromSpritesheet(const State &state, int frameCount, float length, SDL_Texture *texture,
	                                 glm::ivec2 tileSize, glm::ivec2 tileMapPos)
	{
		int stateIndex = 0;
		stateIndex = static_cast<int>(state);
		animations[stateIndex] = Animation(frameCount, length);

		std::vector<AnimationFrame> animationFrames{};
		for (int i = 0; i < frameCount; i++) {
			animationFrames.push_back({texture, tileSize, glm::ivec2(tileMapPos.x + i * tileSize.x, tileMapPos.y)});
		}

		frames[stateIndex] = animationFrames;
	}
};
