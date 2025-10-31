#pragma once
#include "Entity.hpp"
#include <cstdint>
#include <unordered_map>

namespace GameEngine::ECS {
template <typename T>
class ComponentStorage {
  public:
	std::unordered_map<Entity, T> components;

	void add_component(Entity entity, const T &component) { components[entity] = component; }

	bool contains_component(Entity entity) const { return components.find(entity) != components.end(); }

	T &get_component(Entity entity) { return components[entity]; }
};
} // namespace GameEngine::ECS