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

enum class AnimationType {
	None,
	Idle,
	Walking,
	JumpStart,
	FlyingUp,
	JumpReload,
	Falling,
	Landing,
	Attacking,
	Charge,
	Fire,
	Hurting,
	Dead
};

class AnimationComponent {
  public:
	void addAnimation(const AnimationType &type, const std::vector<AnimationFrame> &animationFrames,
	                  const std::chrono::duration<float> duration)
	{
		animations[type] = Animation(static_cast<int>(animationFrames.size()), duration);
		frames[type] = animationFrames;
	}

	void addAnimationFromSpritesheet(const AnimationType &type, const int frameCount,
	                                 const std::chrono::duration<float> length, SDL_Texture *texture,
	                                 const glm::ivec2 tileSize, const glm::ivec2 tileMapPos)
	{
		animations[type] = Animation(frameCount, length);

		std::vector<AnimationFrame> animationFrames{};
		for (int i = 0; i < frameCount; i++) {
			animationFrames.push_back({texture, tileSize, glm::ivec2(tileMapPos.x + i * tileSize.x, tileMapPos.y)});
		}

		frames[type] = animationFrames;
	}

	bool hasAnimation(const AnimationType &type) const { return animations.contains(type); };

	AnimationFrame &currentFrame(const AnimationType &type)
	{
		return frames.at(type).at(animations.at(type).currentFrame());
	}

	bool hasFinished(const AnimationType &type) const
	{
		if (!animations.contains(type)) {
			return true; // when an animation is missing always mark it as finished so it does not block the animation
			             // queue
		}
		return animations.at(type).completed();
	}

	void stepAnimation(const AnimationType &type, const std::chrono::duration<float> deltaTime) { animations.at(type).step(deltaTime); }

	std::chrono::duration<float> getAnimationDuration(const AnimationType &type) const
	{
		if (!animations.contains(type)) {
			return std::chrono::duration<float>::zero();
		}
		return animations.at(type).getLength();
	}

	AnimationType getAnimation() const { return currentAnimation; }
	void setAnimation(const AnimationType &type)
	{
		if (currentAnimation != type) {
			startAnimation(type);
		}
		currentAnimation = type;
	}

  private:
	void startAnimation(const AnimationType &type) { animations.at(type).start(); }
	std::unordered_map<AnimationType, Animation> animations{};
	std::unordered_map<AnimationType, std::vector<AnimationFrame>> frames{};
	AnimationType currentAnimation = AnimationType::None;
};
