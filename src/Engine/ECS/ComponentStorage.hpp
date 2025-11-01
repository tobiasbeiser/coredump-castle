#pragma once
#include "Entity.hpp"
#include <unordered_map>

namespace GameEngine::ECS {
template <typename T>
class ComponentStorage {
  public:
	std::unordered_map<Entity, T> components;

	void add_component(Entity entity, const T &component) { components[entity] = component; }

	void remove_component(Entity entity) { components.erase(entity); }

	bool contains_component(Entity entity) const { return components.contains(entity); }

	T &get_component(Entity entity) { return components[entity]; }
};
} // namespace GameEngine::ECS