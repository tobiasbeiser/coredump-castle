#pragma once
#include "Engine/ECS/Components/Render/AnimationComponent.hpp"

#include <map>
#include <string>

struct SoundComponent {
	std::unordered_map<AnimationType, std::string> sounds{};

	AnimationType lastAnimation = AnimationType::None;

	Timer stepTimer{std::chrono::duration<float>(0.5f)};
};