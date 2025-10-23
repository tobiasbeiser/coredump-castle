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
		// TODO Optimize by iterating the smallest storage
		using FirstComponent = typename std::tuple_element<0, std::tuple<Components...>>::type;

		auto &firstStorage = registry.get_storage<typename std::tuple_element_t<0, std::tuple<Components...>>>();

		for (const auto &[entity, _] : firstStorage.components) {
			if ((registry.get_storage<Components>().contains_component(entity) && ...)) {
				entities.push_back(entity);
			}
		}
	}

	auto begin() { return entities.begin(); }
	auto end() { return entities.end(); }
	auto at(const int index) { return entities.at(index); }

	int size() { return entities.size(); }

	template <typename T>
	T &get(Entity e)
	{
		return registry.get_storage<T>().get_component(e);
	}
};
} // namespace GameEngine::ECS