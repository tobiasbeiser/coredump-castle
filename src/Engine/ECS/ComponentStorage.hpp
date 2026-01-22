#pragma once
#include "Entity.hpp"
#include <unordered_map>

namespace GameEngine::ECS {
template <typename T>
class ComponentStorage {
  public:
	std::unordered_map<Entity, T> components;

	void addComponent(Entity entity, const T &component) { components.insert_or_assign(entity, component); }

	void removeComponent(Entity entity) { components.erase(entity); }

	bool containsComponent(Entity entity) const { return components.contains(entity); }

	T &getComponent(Entity entity) { return components.at(entity); }
};
} // namespace GameEngine::ECS