#pragma once

#include "Engine/ECS/Components/Render/CameraShakeComponent.hpp"
#include "Engine/ECS/Registry.hpp"
#include "Engine/ECS/View.hpp"
#include "Engine/Utils/Random.hpp"

#include <algorithm>

namespace GameEngine::ECS::Systems::CameraShakeSystem {

inline void update(Registry &registry, const float deltaTime)
{
	for (auto view = registry.view<CameraShakeComponent>(); const auto entity : view) {
		auto &shake = view.get<CameraShakeComponent>(entity);
		shake.duration.step(std::chrono::duration<float>(deltaTime), false);

		if (shake.duration.isTimeout()) {
			shake.offset = {0.0f, 0.0f};
			registry.remove<CameraShakeComponent>(entity);
			continue;
		}

		const float ratio = shake.duration.getTime().count() / shake.duration.getLength().count();
		const float intensity = shake.decay ? std::clamp(1.0f - ratio, 0.0f, 1.0f) : 1.0f;
		const float amp = shake.amplitude * intensity;

		shake.offset = {Utils::Random::getFloat(-amp, amp), Utils::Random::getFloat(-amp, amp)};
	}
}

} // namespace GameEngine::ECS::Systems::CameraShakeSystem
