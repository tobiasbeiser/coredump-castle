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
	// DENSE: The actual data, packed tightly.
	// We keep 'denseEntities' to know which Entity owns the component at index [i]
	std::vector<T> denseComponents;
	std::vector<Entity> denseEntities;

	// SPARSE: Maps Entity ID -> Index in dense arrays.
	// sparse[EntityID] = Index
	std::vector<size_t> sparse;

	static constexpr size_t invalidIndex = std::numeric_limits<size_t>::max();

  public:
	// --- Interface for View and Registry ---

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
			// If it already exists, just update it
			denseComponents[sparse[entity]] = component;
			return;
		}

		// Ensure sparse array is big enough
		if (entity >= sparse.size()) {
			sparse.resize(entity + 1, invalidIndex);
		}

		// Add to the end of Dense arrays
		sparse[entity] = denseComponents.size();
		denseComponents.push_back(component);
		denseEntities.push_back(entity);
	}

	void remove(const Entity entity) override
	{
		if (!contains(entity))
			return;

		// SWAP AND POP:
		// To keep the array contiguous without holes, we move the
		// last element into the spot of the removed element.

		size_t indexToRemove = sparse[entity];
		size_t indexLast = denseComponents.size() - 1;
		Entity entityLast = denseEntities[indexLast];

		// 1. Overwrite the removed element with the last element
		denseComponents[indexToRemove] = denseComponents[indexLast];
		denseEntities[indexToRemove] = denseEntities[indexLast];

		// 2. Update the sparse map for the swapped entity
		sparse[entityLast] = indexToRemove;

		// 3. Mark the removed entity as invalid
		sparse[entity] = invalidIndex;

		// 4. Remove the duplicate from the end
		denseComponents.pop_back();
		denseEntities.pop_back();
	}

	std::vector<T> &getComponents() { return denseComponents; }

	[[nodiscard]] size_t getSize() const override { return denseEntities.size(); }
	[[nodiscard]] const std::vector<Entity> &getEntities() const override { return denseEntities; }
};

} // namespace GameEngine::ECS