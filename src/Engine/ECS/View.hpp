#pragma once

#include "Registry.hpp"
#include <tuple>
#include <vector>
namespace GameEngine::ECS {

template <typename... Components>
class View {
	Registry &registry;
	std::vector<Entity> entities;

  public:
	View(Registry &reg) : registry(reg)
	{
		using Tuple = std::tuple<Components...>;
		auto &firstStorage = registry.getStorage<std::tuple_element_t<0, Tuple>>();

		// Cache all storages once
		auto storages = std::make_tuple(&registry.getStorage<Components>()...);

		for (const auto &[entity, _] : firstStorage.components) {
			if ((std::get<ComponentStorage<Components> *>(storages)->components.contains(entity) && ...)) {
				entities.push_back(entity);
			}
		}
	}

	template <typename... Excluded>
	View &exclude()
	{
		std::vector<Entity> filtered;
		for (Entity e : entities) {
			if ((!registry.has<Excluded>(e) && ...)) {
				filtered.push_back(e);
			}
		}
		entities = std::move(filtered);
		return *this;
	}

	auto begin() { return entities.begin(); }
	auto end() { return entities.end(); }
	auto at(const int index) { return entities.at(index); }

	int size() const { return entities.size(); }

	template <typename T>
	T &get(Entity e)
	{
		return registry.getStorage<T>().getComponent(e);
	}
};
} // namespace GameEngine::ECS