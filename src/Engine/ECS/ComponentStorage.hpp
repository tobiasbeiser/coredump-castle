#pragma once
#include "Entity.hpp"
#include "IStorage.hpp"
#include <unordered_map>

namespace GameEngine::ECS {

template <typename T>
class ComponentStorage final : public IStorage {
  public:
	std::unordered_map<Entity, T> components;

	void remove(Entity entity) override { components.erase(entity); }

	void insert(Entity entity, const T &component) { components.insert_or_assign(entity, component); }
	[[nodiscard]] bool contains(Entity entity) const { return components.contains(entity); }

	T &get(Entity entity) { return components.at(entity); }
};
} // namespace GameEngine::ECS