#pragma once

#include "Registry.hpp"
#include <algorithm>
#include <array>
#include <tuple>
#include <vector>

namespace GameEngine::ECS {

template <typename... Components>
class View {
	std::tuple<ComponentStorage<Components> *...> pools;
	std::vector<Entity> entities;
	Registry *registry;

  public:
	explicit View(Registry &reg) : registry(&reg), pools(std::make_tuple(&reg.getStorage<Components>()...))
	{
		std::array<IStorage *, sizeof...(Components)> genericPools = {&reg.getStorage<Components>()...};

		IStorage *smallestPool = genericPools[0];
		size_t smallestSize = smallestPool->getSize();

		for (size_t i = 1; i < genericPools.size(); ++i) {
			if (genericPools[i]->getSize() < smallestSize) {
				smallestPool = genericPools[i];
				smallestSize = smallestPool->getSize();
			}
		}
		entities.reserve(smallestSize);

		const auto &candidateEntities = smallestPool->getEntities();
		for (const auto entity : candidateEntities) {
			if ((std::get<ComponentStorage<Components> *>(pools)->contains(entity) && ...)) {
				entities.push_back(entity);
			}
		}
	}

	template <typename... Excluded>
	View exclude() &&
	{
		auto it = std::remove_if(entities.begin(), entities.end(),
		                         [&](const Entity e) { return (registry->has<Excluded>(e) || ...); });

		entities.erase(it, entities.end());
		return std::move(*this);
	}

	auto begin() { return entities.begin(); }
	auto end() { return entities.end(); }
	[[nodiscard]] int size() const { return entities.size(); }
	auto at(const int index) { return entities.at(index); }

	template <typename T>
	T &get(Entity e)
	{
		return std::get<ComponentStorage<T> *>(pools)->get(e);
	}
};
} // namespace GameEngine::ECS