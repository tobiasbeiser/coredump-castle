//
// Created by Tobias on 25.01.2026.
//

#pragma once

class AudioManager;
namespace GameEngine::ECS {
class Registry;
}
namespace GameEngine::ECS::Systems::SoundSystem {

void update(Registry &registry, AudioManager &audioManager, float deltaTime);

}; // namespace GameEngine::ECS::Systems::SoundSystem
