#pragma once
namespace GameEngine::ECS {
class Registry;
}

namespace GameEngine::ECS::Systems::BlinkSystem {
void update(Registry &registry);
}