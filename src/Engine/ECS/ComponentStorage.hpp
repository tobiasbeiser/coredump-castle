#pragma once
#include "Entity.hpp"
#include "IStorage.hpp"
#include <cassert>
#include <limits>
#include <vector>

namespace GameEngine::ECS {

template <typename T>
class ComponentStorage final : public IStorage {
  private:
	std::vector<T> denseComponents;
	std::vector<Entity> denseEntities;

	std::vector<size_t> sparse;

	static constexpr size_t invalidIndex = std::numeric_limits<size_t>::max();

  public:
	[[nodiscard]] bool contains(const Entity entity) const
	{
		return entity < sparse.size() && sparse[entity] != invalidIndex;
	}

	T &get(const Entity entity)
	{
		assert(contains(entity) && "Entity does not have this component!");
		return denseComponents[sparse[entity]];
	}

	void insert(const Entity entity, const T &component)
	{
		if (contains(entity)) {
			denseComponents[sparse[entity]] = component;
			return;
		}

		if (entity >= sparse.size()) {
			sparse.resize(entity + 1, invalidIndex);
		}

		sparse[entity] = denseComponents.size();
		denseComponents.push_back(component);
		denseEntities.push_back(entity);
	}

	void remove(const Entity entity) override
	{
		if (!contains(entity))
			return;

		size_t indexToRemove = sparse[entity];
		size_t indexLast = denseComponents.size() - 1;
		Entity entityLast = denseEntities[indexLast];

		denseComponents[indexToRemove] = denseComponents[indexLast];
		denseEntities[indexToRemove] = denseEntities[indexLast];

		sparse[entityLast] = indexToRemove;

		sparse[entity] = invalidIndex;

		denseComponents.pop_back();
		denseEntities.pop_back();
	}

	std::vector<T> &getComponents() { return denseComponents; }

	[[nodiscard]] size_t getSize() const override { return denseEntities.size(); }
	[[nodiscard]] const std::vector<Entity> &getEntities() const override { return denseEntities; }
};

} // namespace GameEngine::ECS