#pragma once
#include <cstdint>
#include <limits>

namespace GameEngine::ECS {
using Entity = std::uint32_t;

constexpr Entity invalidEntity = std::numeric_limits<std::uint32_t>::max();
} // namespace GameEngine::ECS