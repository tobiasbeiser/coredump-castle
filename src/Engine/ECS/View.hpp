#pragma once

#include "Registry.hpp"
#include <algorithm>
#include <tuple>
#include <vector>
namespace GameEngine::ECS {

template <typename... Components>
class View {
	Registry *registry;
	std::vector<Entity> entities;

  public:
	explicit View(Registry &reg) : registry(&reg)
	{
		auto storages = std::make_tuple(&reg.getStorage<Components>()...);

		// Get the storage of the first component type (The "Driver")
		auto &driverStorage = *std::get<0>(storages);
		const auto &driverEntities = driverStorage.getEntities(); // <--- Get Vector, not Map

		entities.reserve(driverEntities.size());

		// Linear iteration over a vector!
		for (const auto entity : driverEntities) {
			// Check if this entity exists in all other component pools
			// This is now an array lookup (fast) instead of a hash lookup (slow)
			if ((std::get<ComponentStorage<Components> *>(storages)->contains(entity) && ...)) {
				entities.push_back(entity);
			}
		}
	}

	View(const View &) = delete;
	View &operator=(const View &) = delete;
	View(View &&) = default;
	View &operator=(View &&) = default;
	~View() = default;

	template <typename... Excluded>
	View exclude() &&
	{
		std::vector<Entity> filtered;
		for (Entity e : entities) {
			if ((!registry->has<Excluded>(e) && ...)) {
				filtered.push_back(e);
			}
		}
		entities = std::move(filtered);
		return std::move(*this);
	}

	auto begin() { return entities.begin(); }
	auto end() { return entities.end(); }
	auto at(const int index) { return entities.at(index); }

	[[nodiscard]] int size() const { return entities.size(); }

	template <typename T>
	T &get(Entity e)
	{
		return registry->getStorage<T>().get(e);
	}
};
} // namespace GameEngine::ECS