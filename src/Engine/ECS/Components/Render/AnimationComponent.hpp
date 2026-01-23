#pragma once
#include "AnimationFrame.hpp"
#include "Engine/Graphics/Animation.hpp"
#include <SDL3/SDL_render.h>
#include <glm/vec2.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
	Hurting,
	Dead,
	Charge,
	Fire,
	Interacting
};

class AnimationComponent {
  public:
	void addAnimation(const AnimationType &type, const std::vector<AnimationFrame> &animationFrames,
	                  const std::chrono::duration<float> duration)
	{
		const bool loop = !nonLoopingAnimations.contains(type);
		animations[type] = Animation(static_cast<int>(animationFrames.size()), duration, loop);
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
		if (!animations.contains(type) || !frames.contains(type) || frames.at(type).empty()) {
			for (auto &[animType, frameList] : frames) {
				if (!frameList.empty()) {
					return frameList[0];
				}
			}
			throw std::runtime_error("AnimationComponent has no frames loaded");
		}

		const int frameIndex = animations.at(type).currentFrame();
		auto &frameList = frames.at(type);

		if (frameIndex < 0 || frameIndex >= static_cast<int>(frameList.size())) {
			return frameList[0];
		}

		return frameList[frameIndex];
	}

	bool hasFinished(const AnimationType &type) const
	{
		if (!animations.contains(type)) {
			return true; // when an animation is missing always mark it as finished so it does not block the animation
			             // queue
		}
		return animations.at(type).completed();
	}

	void stepAnimation(const AnimationType &type, const std::chrono::duration<float> deltaTime)
	{
		if (animations.contains(type)) {
			animations.at(type).step(deltaTime);
		}
	}

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
	void startAnimation(const AnimationType &type)
	{
		if (animations.contains(type)) {
			animations.at(type).start();
		}
	}
	std::unordered_map<AnimationType, Animation> animations{};
	std::unordered_map<AnimationType, std::vector<AnimationFrame>> frames{};
	AnimationType currentAnimation = AnimationType::None;
	std::unordered_set<AnimationType> nonLoopingAnimations = {AnimationType::Dead, AnimationType::Hurting,
	                                                          AnimationType::Attacking, AnimationType::Interacting};
};
