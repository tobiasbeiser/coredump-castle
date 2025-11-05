#pragma once
#include "Engine/Graphics/Animation.hpp"
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
	std::vector<std::vector<AnimationFrame>> frames{};
	std::vector<Animation> animations{};
	int currentAnimation = 0;
	int previousAnimation = 0;
	std::unordered_map<int, int> stateToAnimationIndex{};

	void addAnimationFromSpritesheet(const State &state, int frameCount, float length, SDL_Texture *texture,
	                                 glm::ivec2 tileSize, glm::ivec2 tileMapPos)
	{
		int stateIndex = static_cast<int>(state);
		const Animation anim(frameCount, length);
		if (stateIndex >= animations.size()) {
			animations.resize(stateIndex + 1);
		}
		animations[stateIndex] = anim;

		std::vector<AnimationFrame> animationFrames{};

		for (size_t i = 0; i < frameCount; i++) {
			AnimationFrame frame{};
			frame.texture = texture;
			frame.tileSize = tileSize;
			frame.tileMapPosition = glm::ivec2(tileMapPos.x + i * tileSize.x, tileMapPos.y);
			animationFrames.push_back(frame);
		}
		if (stateIndex >= frames.size()) {
			frames.resize(stateIndex + 1);
		}
		frames[stateIndex] = animationFrames;
	}
};