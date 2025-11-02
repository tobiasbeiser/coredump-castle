#pragma once
#include "Engine/Graphics/Animation.hpp"
#include <unordered_map>
#include <vector>
#include <SDL3/SDL_render.h>
#include <glm/vec2.hpp>

struct AnimationFrame {
	SDL_Texture *texture;
	glm::ivec2 TileSize;
	glm::ivec2 TileMapPosition;
};

struct AnimationComponent {
	std::vector<std::vector<AnimationFrame>> frames;
	std::vector<Animation> animations;
	int currentAnimation = 0;
	int previousAnimation = 0;
	std::unordered_map<int, int> stateToAnimationIndex;

	void addAnimationFromSpritesheet(int state, int frameCount, float length, SDL_Texture *texture, glm::ivec2 tileSize,
	                                 glm::ivec2 tileMapPos)
	{
		const Animation anim(frameCount, length);
		if (state >= animations.size()) {
			animations.resize(state + 1);
		}
		animations[state] = anim;

		std::vector<AnimationFrame> animationFrames;

		for (size_t i = 0; i < frameCount; i++) {
			AnimationFrame frame;
			frame.texture = texture;
			frame.TileSize = tileSize;
			frame.TileMapPosition = glm::ivec2(tileMapPos.x + i * tileSize.x, tileMapPos.y);
			animationFrames.push_back(frame);
		}
		if (state >= frames.size()) {
			frames.resize(state + 1);
		}
		frames[state] = animationFrames;
	}
};