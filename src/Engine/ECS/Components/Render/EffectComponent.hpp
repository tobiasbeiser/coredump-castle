#pragma once
#include "AnimationFrame.hpp"
#include "Engine/Graphics/Animation.hpp"
#include <vector>

struct EffectComponent {
	Animation animation;
	std::vector<AnimationFrame> frames;
	bool shouldDestroyOnFinish = true;

	EffectComponent(const std::vector<AnimationFrame> &_frames, const Animation &animation)
	    : animation(animation), frames(_frames)
	{
	}
};